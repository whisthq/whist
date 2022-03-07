package main

import (
	"encoding/json"
	"io"
	"net/http"
	"strings"
	"time"

	"github.com/google/uuid"
	"github.com/whisthq/whist/backend/services/host-service/auth"
	"github.com/whisthq/whist/backend/services/scaling-service/payments"
	algos "github.com/whisthq/whist/backend/services/scaling-service/scaling_algorithms/default"
	"github.com/whisthq/whist/backend/services/utils"
	logger "github.com/whisthq/whist/backend/services/whistlogger"
	"golang.org/x/time/rate"
)

// A ServerRequest represents a request from the server --- it is exported so
// that we can implement the top-level event handlers in parent packages. They
// simply return the result and any error message via ReturnResult.
type ServerRequest interface {
	ReturnResult(result interface{}, err error)
	createResultChan()
}

// A requestResult represents the result of a request that was successfully
// authenticated, parsed, and processed by the consumer.
type requestResult struct {
	Result interface{} `json:"-"`
	Err    error       `json:"error"`
}

type MandelboxAssignRequest struct {
	Regions    []string           `json:"regions"`
	CommitHash string             `json:"client_commit_hash"`
	SessionID  int64              `json:"session_id"`
	UserEmail  string             `json:"user_email"`
	resultChan chan requestResult // Channel to pass the request result between goroutines
}

type MandelboxAssignRequestResult struct {
}

// ReturnResult is called to pass the result of a request back to the HTTP
// request handler.
func (s *MandelboxAssignRequest) ReturnResult(result interface{}, err error) {
	s.resultChan <- requestResult{result, err}
}

// createResultChan is called to create the Go channel to pass the request
// result back to the HTTP request handler via ReturnResult.
func (s *MandelboxAssignRequest) createResultChan() {
	if s.resultChan == nil {
		s.resultChan = make(chan requestResult)
	}
}

// send is called to send an HTTP response
func (r requestResult) send(w http.ResponseWriter) {
	var buf []byte
	var err error
	var status int

	if r.Err != nil {
		// Send a 406
		status = http.StatusNotAcceptable
		buf, err = json.Marshal(
			struct {
				Result interface{} `json:"result"`
				Error  string      `json:"error"`
			}{r.Result, r.Err.Error()},
		)
	} else {
		// Send a 200 code
		status = http.StatusOK
		buf, err = json.Marshal(
			struct {
				Result interface{} `json:"result"`
			}{r.Result},
		)
	}

	w.WriteHeader(status)
	if err != nil {
		logger.Errorf("Error marshalling a %v HTTP Response body: %s", status, err)
	}
	_, _ = w.Write(buf)
}

func MandelboxAssignHandler(w http.ResponseWriter, req *http.Request, events chan<- algos.ScalingEvent) {
	// Verify that we got a POST request
	err := verifyRequestType(w, req, http.MethodPost)
	if err != nil {
		logger.Errorf("Error verifying request type. Err: %v", err)
	}

	var reqdata MandelboxAssignRequest
	err = authenticateAndParseRequest(w, req, &reqdata)
	if err != nil {
		logger.Errorf("Failed while authenticating request. Err: %v", err)
	}

	// Once we have authenticated and validated the request
	// send it to the scaling algorithm for processing
	events <- algos.ScalingEvent{
		ID:   uuid.NewString(),
		Type: "SERVER_MANDELBOX_ASSIGN_EVENT",
		Data: reqdata,
	}
	res := <-reqdata.resultChan

	res.send(w)
}

func authenticateAndParseRequest(w http.ResponseWriter, r *http.Request, s ServerRequest) error {
	accessToken := r.Header.Get("Authorization")
	accessToken = strings.Split(accessToken, "Bearer ")[1]

	claims, err := auth.ParseToken(accessToken)
	if err != nil {
		http.Error(w, "Unauthorized", http.StatusUnauthorized)
		return utils.MakeError("Received an unpermissioned backend request on %s to URL %s. Error: %s", r.Host, r.URL, err)
	}
	logger.Infof("claims are %v", claims)

	// Get request body
	body, err := io.ReadAll(r.Body)
	if err != nil {
		http.Error(w, "Malformed body", http.StatusBadRequest)
		return utils.MakeError("Error getting body from request on %s to URL %s: %s", r.Host, r.URL, err)
	}

	// Unmarshal request body into interface result
	err = json.Unmarshal(body, s)
	if err != nil {
		return utils.MakeError("Failed to unmarshal request body. Err: %v", err)
	}

	s.createResultChan()

	return nil
}

// throttleMiddleware will limit requests on the endpoint using the  provided rate limiter.
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

	// err := initializeTLS()
	// if err != nil {
	// 	logger.Errorf("Error starting HTTP Server: %v", err)
	// }

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

	// Add timeouts to help mitigate potential rogue clients
	// or DDOS attacks.
	srv := &http.Server{
		Addr:         "0.0.0.0:8082",
		ReadTimeout:  5 * time.Second,
		WriteTimeout: 5 * time.Second,
		IdleTimeout:  120 * time.Second,
		Handler:      mux,
	}

	go func() {
		logger.Error(srv.ListenAndServe())
	}()
}
