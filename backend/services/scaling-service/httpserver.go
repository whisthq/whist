package main

import (
	"encoding/json"
	"io"
	"net/http"
	"strings"
	"time"

	"github.com/whisthq/whist/backend/services/host-service/auth"
	scaling_algorithms "github.com/whisthq/whist/backend/services/scaling-service/scaling_algorithms/default"
	"github.com/whisthq/whist/backend/services/utils"
	logger "github.com/whisthq/whist/backend/services/whistlogger"
)

type MandelboxAssignRequest struct {
	Regions    []string `json:"regions"`
	CommitHash string   `json:"client_commit_hash"`
	SessionID  int64    `json:"session_id"`
	UserEmail  string   `json:"user_email"`
}

func AssignHandler(w http.ResponseWriter, req *http.Request, events chan<- scaling_algorithms.ScalingEvent) {
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
}

func authenticateAndParseRequest(w http.ResponseWriter, r *http.Request, event interface{}) error {
	accessToken := r.Header.Get("Authorization")
	accessToken = strings.Split(accessToken, "Bearer ")[1]

	logger.Infof("Validating access token: %v", accessToken)
	claims, err := auth.ParseToken(accessToken)
	if err != nil {
		http.Error(w, "Unauthorized", http.StatusUnauthorized)
		return utils.MakeError("Received an unpermissioned backend request on %s to URL %s. Error: %s", r.Host, r.URL, err)
	}
	logger.Infof("Claims are %v", claims)

	// Get request body
	body, err := io.ReadAll(r.Body)
	if err != nil {
		http.Error(w, "Malformed body", http.StatusBadRequest)
		return utils.MakeError("Error getting body from request on %s to URL %s: %s", r.Host, r.URL, err)
	}

	// Extract only the jwt_access_token field from a raw JSON unmarshalling that
	// delays as much decoding as possible
	var rawmap map[string]*json.RawMessage
	err = json.Unmarshal(body, &rawmap)
	if err != nil {
		http.Error(w, "Malformed body", http.StatusBadRequest)
		return utils.MakeError("Error raw-unmarshalling JSON body sent on %s to URL %s: %s", r.Host, r.URL, err)
	}

	logger.Infof("rawmap is %v", rawmap)
	err = json.Unmarshal(body, event)
	if err != nil {
		return utils.MakeError("Failed to unmarshal request body. Err: %v", err)
	}

	logger.Infof("Unmarshalled into %v", event)
	return nil
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

func StartHTTPServer() {
	logger.Infof("Starting HTTP server...")

	// err := initializeTLS()
	// if err != nil {
	// 	logger.Errorf("Error starting HTTP Server: %v", err)
	// }

	// Buffer up to 100 events so we don't block. This should be mostly
	// unnecessary, but we want to be able to handle a burst without blocking.
	events := make(chan scaling_algorithms.ScalingEvent, 100)

	createHandler := func(f func(http.ResponseWriter, *http.Request, chan<- scaling_algorithms.ScalingEvent)) func(http.ResponseWriter, *http.Request) {
		return func(w http.ResponseWriter, r *http.Request) {
			f(w, r, events)
		}
	}

	// Create a custom HTTP Request Multiplexer
	mux := http.NewServeMux()
	mux.Handle("/", http.NotFoundHandler())
	mux.HandleFunc("/mandelbox/assign", createHandler(AssignHandler))

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
