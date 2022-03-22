package main

import (
	"encoding/json"
	"net/http"
	"strings"
	"time"

	"github.com/google/uuid"
	"github.com/whisthq/whist/backend/services/host-service/auth"
	"github.com/whisthq/whist/backend/services/httputils"
	"github.com/whisthq/whist/backend/services/scaling-service/payments"
	algos "github.com/whisthq/whist/backend/services/scaling-service/scaling_algorithms/default"
	"github.com/whisthq/whist/backend/services/types"
	"github.com/whisthq/whist/backend/services/utils"
	logger "github.com/whisthq/whist/backend/services/whistlogger"
	"golang.org/x/time/rate"
)

// Constants for use in setting up the HTTPS server
const (
	PortToListen   uint16 = 7730
	certPath       string = "cert.pem"
	privatekeyPath string = "key.pem"
)

func MandelboxAssignHandler(w http.ResponseWriter, req *http.Request, events chan<- algos.ScalingEvent) {
	// Verify that we got a POST request
	err := verifyRequestType(w, req, http.MethodPost)
	if err != nil {
		logger.Errorf("Error verifying request type. Err: %v", err)
	}

	var reqdata httputils.MandelboxAssignRequest
	err = authenticateRequest(w, req, &reqdata)
	if err != nil {
		logger.Errorf("Failed while authenticating request. Err: %v", err)
	}

	// Once we have authenticated and validated the
	// request send it to the scaling algorithm for
	// processing. Since this event does not deal with
	// AWS directly, send to the default us-east-1 scaling algorithm.
	events <- algos.ScalingEvent{
		ID:     uuid.NewString(),
		Type:   "SERVER_MANDELBOX_ASSIGN_EVENT",
		Region: "us-east-1",
		Data:   reqdata,
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

func authenticateRequest(w http.ResponseWriter, r *http.Request, s httputils.ServerRequest) error {
	accessToken := r.Header.Get("Authorization")
	accessToken = strings.Split(accessToken, "Bearer ")[1]

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
		accessToken := r.Header.Get("Authorization")
		accessToken = strings.Split(accessToken, "Bearer ")[1]

		// Verify if the user's subscription is valid
		paymentValid, err := payments.VerifyPayment(accessToken)
		if err != nil {
			logger.Errorf("Failed to validate Stripe payment from access token. Err: %v", err)
			http.Error(rw, http.StatusText(http.StatusInternalServerError), http.StatusInternalServerError)
		}

		if !paymentValid {
			logger.Warningf("User does not have an active Stripe subscription.")
			http.Error(rw, http.StatusText(http.StatusForbidden), http.StatusForbidden)
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

func StartHTTPServer(events chan algos.ScalingEvent) {
	logger.Infof("Starting HTTP server...")

	err := httputils.InitializeTLS(certPath, privatekeyPath)
	if err != nil {
		logger.Errorf("Error starting HTTP Server: %v", err)
	}

	createHandler := func(f func(http.ResponseWriter, *http.Request, chan<- algos.ScalingEvent)) func(http.ResponseWriter, *http.Request) {
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
	assignHandler := verifyPaymentMiddleware(throttleMiddleware(limiter, createHandler(MandelboxAssignHandler)))

	// Create a custom HTTP Request Multiplexer
	mux := http.NewServeMux()
	mux.Handle("/", http.NotFoundHandler())
	mux.Handle("/mandelbox/assign", assignHandler)

	// Set read/write timeouts to help mitigate potential rogue clients
	// or DDOS attacks.
	srv := &http.Server{
		Addr:         utils.Sprintf("0.0.0.0:%v", PortToListen),
		ReadTimeout:  5 * time.Second,
		WriteTimeout: 5 * time.Second,
		IdleTimeout:  120 * time.Second,
		Handler:      mux,
	}

	go func() {
		logger.Error(srv.ListenAndServe())
	}()
}
