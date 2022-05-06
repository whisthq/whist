// Copyright (c) 2021-2022 Whist Technologies, Inc.

/*
This file contains the code related to starting the scaling service's HTTP server,
and functions for handling the different types of requests. The implementation is
simple, but it has features that make it more robust in case of misuse or attacks
(such as rate limiting, read and write time outs).

The request handlers are responsible for authentication, verification, parsing, etc.
and perform minimal processing of the data. After doing so they send the event through
the server's channel so the event handler can send it to the scaling algorithms in turn.

Some of the logic and types are found on the `httputils` package, which is shared between
scaling service and host service.
*/

package main

import (
	"encoding/json"
	"net/http"
	"os"
	"strings"
	"time"

	"github.com/google/uuid"
	"github.com/whisthq/whist/backend/services/host-service/auth"
	"github.com/whisthq/whist/backend/services/httputils"
	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/scaling-service/algorithms"
	"github.com/whisthq/whist/backend/services/scaling-service/payments"
	"github.com/whisthq/whist/backend/services/subscriptions"
	"github.com/whisthq/whist/backend/services/types"
	"github.com/whisthq/whist/backend/services/utils"
	logger "github.com/whisthq/whist/backend/services/whistlogger"
	"golang.org/x/time/rate"
)

// mandelboxAssignHandler is the http handler for any requests to the `/assign` endpoint. It authenticates
// the request, verifies the type, and parses the body. After that it sends it to the server events channel.
func mandelboxAssignHandler(w http.ResponseWriter, req *http.Request, events chan<- algorithms.ScalingEvent) {
	// Verify that we got a POST request
	err := verifyRequestType(w, req, http.MethodPost)
	if err != nil {
		logger.Errorf("Error verifying request type. Err: %v", err)
		http.Error(w, http.StatusText(http.StatusBadRequest), http.StatusBadRequest)
		return
	}

	var reqdata httputils.MandelboxAssignRequest
	err = authenticateRequest(w, req, &reqdata)
	if err != nil {
		logger.Errorf("Failed while authenticating request. Err: %v", err)
		return
	}

	// Once we have authenticated and validated the request send it to the scaling
	// algorithm for processing. Mandelbox assign is region-agnostic so we don't need
	// to specify a region on the event. The event handler will select the default region automatically.
	events <- algorithms.ScalingEvent{
		ID:   uuid.NewString(),
		Type: "SERVER_MANDELBOX_ASSIGN_EVENT",
		Data: &reqdata,
	}
	res := <-reqdata.ResultChan
	assignResult := res.Result.(httputils.MandelboxAssignRequestResult)

	var (
		buf    []byte
		status int
	)

	if assignResult.Error != "" {
		// Send a 503
		status = http.StatusServiceUnavailable
	} else {
		// Send a 200 code
		status = http.StatusOK
	}

	buf, err = json.Marshal(assignResult)
	w.WriteHeader(status)
	if err != nil {
		logger.Errorf("Error marshalling a %v HTTP Response body: %s", status, err)
	}
	_, _ = w.Write(buf)
}

// paymentsHandler handles a payment session request. It verifies the access token,
// and then creates a StripeClient to get the URL of a payment session.
func paymentsHandler(w http.ResponseWriter, req *http.Request) {
	// Verify that we got a GET request
	err := verifyRequestType(w, req, http.MethodGet)
	if err != nil {
		// err is already logged
		return
	}

	// Extract access token from request header
	accessToken, err := getAccessToken(req)
	if err != nil {
		logger.Error(err)
		http.Error(w, "Did not receive an access token", http.StatusUnauthorized)
		return
	}

	// Get claims from access token
	claims, err := auth.ParseToken(accessToken)
	if err != nil {
		logger.Errorf("Received an unpermissioned backend request on %s to URL %s. Error: %s", req.Host, req.URL, err)
		http.Error(w, "Invalid access token", http.StatusUnauthorized)
		return
	}

	// Setup a config database GraphQL client to get the Stripe configurations.
	// The client is simply passed to the payments client, no database operations
	// are done here.
	useConfigDB := true

	configGraphqlClient := &subscriptions.GraphQLClient{}
	err = configGraphqlClient.Initialize(useConfigDB)
	if err != nil {
		logger.Errorf("Failed to setup config GraphQL client. Err: %v", err)
		http.Error(w, "", http.StatusInternalServerError)
	}

	// Create a new StripeClient to handle the customer
	paymentsClient := &payments.PaymentsClient{}
	stripeClient := &payments.StripeClient{}
	err = paymentsClient.Initialize(claims.CustomerID, claims.SubscriptionStatus, configGraphqlClient, stripeClient)
	if err != nil {
		logger.Errorf("Failed to Initialize Stripe Client. Err: %v", err)
		http.Error(w, "", http.StatusInternalServerError)
		return
	}

	sessionUrl, err := paymentsClient.CreateSession()
	if err != nil {
		logger.Errorf("Failed to create Stripe Session. Err: %v", err)
		http.Error(w, "", http.StatusInternalServerError)
		return
	}

	// Marshal and send session URL
	buf, err := json.Marshal(map[string]string{
		"url": sessionUrl,
	})
	if err != nil {
		logger.Errorf("Error marshalling HTTP Response body: %s", err)
		http.Error(w, "", http.StatusInternalServerError)
		return
	}

	w.WriteHeader(http.StatusOK)
	_, _ = w.Write(buf)
}

// authenticateRequest will verify that the access token is valid
// and will parse the request body and try to unmarshal into a
// `ServerRequest` type.
func authenticateRequest(w http.ResponseWriter, r *http.Request, s httputils.ServerRequest) error {
	accessToken, err := getAccessToken(r)
	if err != nil {
		return err
	}

	claims, err := auth.ParseToken(accessToken)
	if err != nil {
		http.Error(w, "Unauthorized", http.StatusUnauthorized)
		return utils.MakeError("Received an unpermissioned backend request on %s to URL %s. Error: %s", r.Host, r.URL, err)
	}

	_, err = httputils.ParseRequest(w, r, s)
	if err != nil {
		return utils.MakeError("Error while parsing request. Err: %v", err)
	}

	// Add user id to the request. This way we don't expose the
	// access token to other processes that don't need access to it.
	s.(*httputils.MandelboxAssignRequest).UserID = types.UserID(claims.Subject)
	return nil
}

// getAccessToken is a helper function that extracts the access token
// from the request "Authorization" header.
func getAccessToken(r *http.Request) (string, error) {
	authorization := r.Header.Get("Authorization")
	bearer := strings.Split(authorization, "Bearer ")
	if len(bearer) <= 1 {
		return "", utils.MakeError("Bearer token is empty.")
	}
	accessToken := bearer[1]
	return accessToken, nil
}

// throttleMiddleware will limit requests on the endpoint using the provided rate limiter.
// It uses a token bucket algorithm, so that every interval of time the "bucket" will refill
// and continue to serve tokens up to a maximum defined by the burst capacity. In case the
// limit is exceeded, return a http 429 error (too many requests).
func throttleMiddleware(limiter *rate.Limiter, f func(http.ResponseWriter, *http.Request)) func(http.ResponseWriter, *http.Request) {
	return http.HandlerFunc(func(rw http.ResponseWriter, r *http.Request) {
		if !limiter.Allow() {
			http.Error(rw, http.StatusText(http.StatusTooManyRequests), http.StatusTooManyRequests)
		}
		f(rw, r)
	})
}

// verifyPaymentMiddleware will verify if the user has an active subscription in Stripe. This
// middleware should be used to restrict endpoint for paying users only.
func verifyPaymentMiddleware(f func(http.ResponseWriter, *http.Request)) http.Handler {
	return http.HandlerFunc(func(rw http.ResponseWriter, r *http.Request) {
		// Get and parse Authorization header from access token
		accessToken, err := getAccessToken(r)
		if err != nil {
			logger.Error(err)
			http.Error(rw, http.StatusText(http.StatusUnauthorized), http.StatusUnauthorized)
			return
		}

		if len(strings.Split(accessToken, "Bearer")) <= 1 {
			logger.Infof("Access token does not have Bearer header. Trying to parse token as is.")
		} else {
			accessToken = strings.Split(accessToken, "Bearer ")[1]
		}

		// Verify if the user's subscription is valid
		paymentValid, err := payments.VerifyPayment(accessToken)
		if err != nil {
			logger.Errorf("Failed to validate Stripe payment from access token. Err: %v", err)
			http.Error(rw, http.StatusText(http.StatusInternalServerError), http.StatusInternalServerError)
			return
		}

		if !paymentValid {
			logger.Warningf("User does not have an active Stripe subscription.")
			http.Error(rw, http.StatusText(http.StatusForbidden), http.StatusForbidden)
			return
		}
		f(rw, r)
	})
}

// Function to verify the type (method) of a request
func verifyRequestType(w http.ResponseWriter, r *http.Request, method string) error {
	if r == nil {
		err := utils.MakeError("Received a nil request expecting to be type %s", method)
		logger.Error(err)

		http.Error(w, utils.Sprintf("Bad request. Expected %s, got nil", method), http.StatusBadRequest)

		return err
	}

	if r.Method != method {
		err := utils.MakeError("Received a request on %s to URL %s of type %s, but it should have been type %s", r.Host, r.URL, r.Method, method)
		logger.Error(err)

		http.Error(w, utils.Sprintf("Bad request type. Expected %s, got %s", method, r.Method), http.StatusBadRequest)

		return err
	}
	return nil
}

// StartHTTPServer will start the server with the necessary configurations and
// assign the handlers to each endpoint.
func StartHTTPServer(events chan algorithms.ScalingEvent) {
	logger.Infof("Starting HTTP server...")

	createHandler := func(f func(http.ResponseWriter, *http.Request, chan<- algorithms.ScalingEvent)) func(http.ResponseWriter, *http.Request) {
		return func(w http.ResponseWriter, r *http.Request) {
			f(w, r, events)
		}
	}

	// Start a new rate limiter. This will limit requests on an endpoint
	//  to every `interval` with a burst of up to `burst` requests. This
	// will help mitigate Denial of Service attacks, or a client app
	// spamming too many requests.
	interval := 1 * time.Second
	burst := 10
	limiter := rate.NewLimiter(rate.Every(interval), burst)

	// Create the final assign handler, with the necessary middleware
	assignHandler := verifyPaymentMiddleware(throttleMiddleware(limiter, createHandler(mandelboxAssignHandler)))

	// Create a custom HTTP Request Multiplexer
	mux := http.NewServeMux()
	mux.Handle("/", http.NotFoundHandler())
	mux.Handle("/mandelbox/assign", assignHandler)
	mux.Handle("/payment_portal_url", http.HandlerFunc(paymentsHandler))

	// The PORT env var will be automatically set by Heroku.
	// If running on localdev, use default port.
	port := os.Getenv("PORT")
	if metadata.IsLocalEnv() || port == "" {
		port = "7730"
	}

	// Set read/write timeouts to help mitigate potential rogue clients
	// or DDOS attacks.
	srv := &http.Server{
		Addr:         utils.Sprintf("0.0.0.0:%v", port),
		ReadTimeout:  5 * time.Second,
		WriteTimeout: 5 * time.Second,
		IdleTimeout:  120 * time.Second,
		Handler:      mux,
	}

	go func() {
		logger.Error(srv.ListenAndServe())
	}()
}
