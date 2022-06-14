package main

import (
	"context"
	"net/http"
	"strings"
	"sync"
	"time"

	"github.com/golang-jwt/jwt/v4"
	"github.com/whisthq/whist/backend/services/host-service/auth"
	"github.com/whisthq/whist/backend/services/host-service/mandelbox/portbindings"
	"github.com/whisthq/whist/backend/services/host-service/metrics"
	"github.com/whisthq/whist/backend/services/httputils"
	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/subscriptions"
	mandelboxtypes "github.com/whisthq/whist/backend/services/types"
	"github.com/whisthq/whist/backend/services/utils"
	logger "github.com/whisthq/whist/backend/services/whistlogger"
)

// Constants for use in setting up the HTTPS server
const (
	// Our HTTPS server listens on the port "HOST" converted to a telephone
	// number.
	PortToListen   uint16 = 4678
	certPath       string = utils.WhistPrivateDir + "cert.pem"
	privatekeyPath string = utils.WhistPrivateDir + "key.pem"
)

func init() {
	portbindings.Reserve(PortToListen, portbindings.TransportProtocolTCP)
}

// processJSONDataRequest processes an HTTP request to receive data
// directly from the client app. It is handled in host-service.go
func processJSONDataRequest(w http.ResponseWriter, r *http.Request, queue chan<- httputils.ServerRequest) {
	// Start timer to measure average time processing http requests.
	start := time.Now()

	// Verify that it is an PUT request
	if httputils.VerifyRequestType(w, r, http.MethodPut) != nil {
		metrics.Increment("FailedRequests")
		return
	}

	// Verify authorization and unmarshal into the right object type
	var reqdata httputils.JSONTransportRequest
	if err := authenticateRequest(w, r, &reqdata, !metadata.IsLocalEnv()); err != nil {
		logger.Errorf("Error authenticating and parsing %T: %s", reqdata, err)
		metrics.Increment("FailedRequests")
		metrics.Increment("ErrorRate")
		return
	}

	// Send request to queue, then wait for result
	queue <- &reqdata
	res := <-reqdata.ResultChan

	res.Send(w)

	// Measure elapsed milliseconds and send to metrics.
	elapsed := time.Since(start)
	metrics.Increment("SuccessfulRequests")
	metrics.Add("TotalRequestTimeMS", elapsed.Milliseconds())
}

// handleJSONTransportRequest handles any incoming JSON transport requests. First it validates the JWT, then it verifies if
// the json data for the particular user exists, and finally it sends the data through the transport request map.
func handleJSONTransportRequest(serverevent httputils.ServerRequest, transportRequestMap map[mandelboxtypes.MandelboxID]chan *httputils.JSONTransportRequest, transportMapLock *sync.Mutex) {
	// Receive the value of the `json_transport request`
	req := serverevent.(*httputils.JSONTransportRequest)

	// First we validate the JWT received from the `json_transport` endpoint
	// Set up auth
	claims := new(auth.WhistClaims)
	parser := &jwt.Parser{SkipClaimsValidation: true}

	// Only verify auth in non-local environments
	if !metadata.IsLocalEnv() {
		// Decode the access token without validating any of its claims or
		// verifying its signature because we've already done that in
		// `authenticateAndParseRequest`. All we want to know is the value of the
		// sub (subject) claim.
		if _, _, err := parser.ParseUnverified(string(req.JwtAccessToken), claims); err != nil {
			logger.Errorf("There was a problem while parsing the access token for the second time: %s", err)
			metrics.Increment("ErrorRate")
			return
		}
	}

	// If the jwt was valid, the next step is to verify if we have already received the JSONData from
	// this specific user. In case we have, we ignore the request. Otherwise we add the request to the map.
	// By performing this validation, we make the host service resistant to DDOS attacks, and by keeping track
	// of each user's json data, we avoid any race condition that might corrupt other mandelboxes.

	// Acquire lock on transport requests map
	transportMapLock.Lock()
	defer transportMapLock.Unlock()

	if transportRequestMap[req.MandelboxID] == nil {
		transportRequestMap[req.MandelboxID] = make(chan *httputils.JSONTransportRequest, 1)
	}

	// Send the JSONTransportRequest data through the map's channel and close the channel to
	// prevent more requests from being sent, this ensures we only receive the
	// json transport request once per user.
	transportRequestMap[req.MandelboxID] <- req
	close(transportRequestMap[req.MandelboxID])
}

// getJSONTransportRequestChannel returns the JSON transport request for the solicited user
// in a safe way. It also creates the channel in case it doesn't exists for that particular user.
func getJSONTransportRequestChannel(mandelboxID mandelboxtypes.MandelboxID,
	transportRequestMap map[mandelboxtypes.MandelboxID]chan *httputils.JSONTransportRequest, transportMapLock *sync.Mutex) chan *httputils.JSONTransportRequest {
	// Acquire lock on transport requests map
	transportMapLock.Lock()
	defer transportMapLock.Unlock()

	req := transportRequestMap[mandelboxID]

	if req == nil {
		transportRequestMap[mandelboxID] = make(chan *httputils.JSONTransportRequest, 1)
	}

	return transportRequestMap[mandelboxID]
}

func getAppName(mandelboxSubscription subscriptions.Mandelbox, transportRequestMap map[mandelboxtypes.MandelboxID]chan *httputils.JSONTransportRequest,
	transportMapLock *sync.Mutex) (*httputils.JSONTransportRequest, mandelboxtypes.AppName) {

	var AppName mandelboxtypes.AppName
	var req *httputils.JSONTransportRequest

	if metadata.IsLocalEnvWithoutDB() {
		// Receive the json transport request immediately when running on local env
		jsonchan := getJSONTransportRequestChannel(mandelboxSubscription.ID, transportRequestMap, transportMapLock)

		// We will wait 1 minute to get the JSON transport request
		select {
		case transportRequest := <-jsonchan:
			req = transportRequest

			if req.AppName == "" {
				// If no app name is set, we default to using the `browsers/chrome` image.
				AppName = mandelboxtypes.AppName("browsers/chrome")
			} else {
				AppName = req.AppName
			}
		case <-time.After(1 * time.Minute):
			return nil, AppName
		}

	} else {
		// If not on a local environment, we pull the app name from the database event.
		// We need to append the "browsers/" prefix.
		appName := utils.Sprintf("browsers/%s", strings.ToLower(mandelboxSubscription.App))
		AppName = mandelboxtypes.AppName(appName)
	}

	return req, AppName
}

// authenticateRequest will verify that the access token is valid
// and will parse the request body and try to unmarshal into a
// `ServerRequest` type.
func authenticateRequest(w http.ResponseWriter, r *http.Request, s httputils.ServerRequest, authorizeAsBackend bool) (err error) {
	// Extract access token from request header
	accessToken, err := httputils.GetAccessToken(r)
	if err != nil {
		logger.Error(err)

		return utils.MakeError("Did not receive an access token: %s", err)
	}

	if err != nil {
		http.Error(w, "Unauthorized", http.StatusUnauthorized)
		return utils.MakeError("Error getting jwt_access_token from JSON body sent on %s to URL %s: %s", r.Host, r.URL, err)
	}

	// Actually verify authentication. We check that the access token sent is a valid JWT signed by Auth0.
	// Parses a raw access token string, verifies the token's signature, ensures that it is valid at the current moment in time.
	claims, err := auth.ParseToken(accessToken)
	if err != nil {
		http.Error(w, "Unauthorized", http.StatusUnauthorized)
		return utils.MakeError("Received an unpermissioned backend request on %s to URL %s. Error: %s", r.Host, r.URL, err)
	}

	if err := auth.Verify(claims); err != nil {
		http.Error(w, "Unauthorized", http.StatusUnauthorized)
		return utils.MakeError("Received an unpermissioned backend request on %s to URL %s. Error: %s", r.Host, r.URL, err)
	}

	return nil
}

// StartHTTPServer returns a channel of events from the HTTP server as its first return value
func StartHTTPServer(globalCtx context.Context, globalCancel context.CancelFunc, goroutineTracker *sync.WaitGroup) (<-chan httputils.ServerRequest, error) {
	logger.Info("Setting up HTTP server.")

	err := httputils.InitializeTLS(privatekeyPath, certPath)
	if err != nil {
		return nil, utils.MakeError("Error starting HTTP Server: %v", err)
	}

	// Buffer up to 100 events so we don't block. This should be mostly
	// unnecessary, but we want to be able to handle a burst without blocking.
	events := make(chan httputils.ServerRequest, 100)

	createHandler := func(f func(http.ResponseWriter, *http.Request, chan<- httputils.ServerRequest)) func(http.ResponseWriter, *http.Request) {
		return func(w http.ResponseWriter, r *http.Request) {
			f(w, r, events)
		}
	}

	jsonTransportHandler := httputils.EnableCORS(createHandler(processJSONDataRequest))
	// Create a custom HTTP Request Multiplexer
	mux := http.NewServeMux()
	mux.Handle("/", http.NotFoundHandler())
	mux.HandleFunc("/json_transport", jsonTransportHandler)

	// Create the server itself
	server := &http.Server{
		Addr:    utils.Sprintf("0.0.0.0:%v", PortToListen),
		Handler: mux,
	}

	// Start goroutine that shuts down `server` if the global context gets
	// cancelled.
	goroutineTracker.Add(1)
	go func() {
		defer goroutineTracker.Done()

		// Start goroutine that actually listens for requests
		goroutineTracker.Add(1)
		go func() {
			defer goroutineTracker.Done()

			if err := server.ListenAndServeTLS(certPath, privatekeyPath); err != nil && err.Error() != "http: Server closed" {
				close(events)
				logger.Panicf(globalCancel, "Error listening and serving in httpserver: %s", err)
			}
		}()

		// Listen for global context cancellation
		<-globalCtx.Done()

		logger.Infof("Shutting down httpserver...")
		shutdownCtx, shutdownCancel := context.WithTimeout(globalCtx, 30*time.Second)
		defer shutdownCancel()

		err := server.Shutdown(shutdownCtx)
		if err != nil {
			logger.Infof("Shut down httpserver with error %s", err)
		} else {
			logger.Info("Gracefully shut down httpserver.")
		}
	}()

	return events, nil
}
