package main

import (
	"context"
	"encoding/json"
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

// JSONTransportRequest defines the (unauthenticated) `json_transport`
// endpoint.
type JSONTransportRequest struct {
	AppName               mandelboxtypes.AppName               `json:"app_name,omitempty"`             // The app name to spin up (used when running in localdev, but in deployment the app name is set to browsers/chrome).
	ConfigEncryptionToken mandelboxtypes.ConfigEncryptionToken `json:"config_encryption_token"`        // User-specific private encryption token
	JwtAccessToken        string                               `json:"jwt_access_token"`               // User's JWT access token
	MandelboxID           mandelboxtypes.MandelboxID           `json:"mandelbox_id"`                   // MandelboxID, used for the json transport request map
	IsNewConfigToken      bool                                 `json:"is_new_config_encryption_token"` // Flag indicating we should expect a new config encryption token and to skip config decryption this run
	JSONData              mandelboxtypes.JSONData              `json:"json_data"`                      // Arbitrary stringified JSON data to pass to mandelbox
	CookiesJSON           mandelboxtypes.Cookies               `json:"cookies,omitempty"`              // The cookies provided by the client-app as JSON string
	BookmarksJSON         mandelboxtypes.Bookmarks             `json:"bookmarks,omitempty"`            // The bookmarks provided by the client-app as JSON string
	Extensions            mandelboxtypes.Extensions            `json:"extensions,omitempty"`           // Extensions provided by the client-app
	Preferences           mandelboxtypes.Preferences           `json:"preferences,omitempty"`          // Preferences provided by the client-app as JSON string
	resultChan            chan httputils.RequestResult         // Channel to pass the request result between goroutines
	LocalStorageJSON      mandelboxtypes.LocalStorage          `json:"local_storage,omitempty"` // Local storage provided by the client-app as JSON string
}

// JSONTransportRequestResult defines the data returned by the
// `json_transport` endpoint.
type JSONTransportRequestResult struct {
	HostPortForTCP32262 uint16 `json:"port_32262"`
	HostPortForUDP32263 uint16 `json:"port_32263"`
	HostPortForTCP32273 uint16 `json:"port_32273"`
	AesKey              string `json:"aes_key"`
}

// ReturnResult is called to pass the result of a request back to the HTTP
// request handler.
func (s *JSONTransportRequest) ReturnResult(result interface{}, err error) {
	s.resultChan <- httputils.RequestResult{
		Result: result,
		Err:    err,
	}
}

// createResultChan is called to create the Go channel to pass the request
// result back to the HTTP request handler via ReturnResult.
func (s *JSONTransportRequest) CreateResultChan() {
	if s.resultChan == nil {
		s.resultChan = make(chan httputils.RequestResult)
	}
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
	var reqdata JSONTransportRequest
	if err := authenticateRequest(w, r, &reqdata, !metadata.IsLocalEnv()); err != nil {
		logger.Errorf("Error authenticating and parsing %T: %s", reqdata, err)
		metrics.Increment("FailedRequests")
		metrics.Increment("ErrorRate")
		return
	}

	// Send request to queue, then wait for result
	queue <- &reqdata
	res := <-reqdata.resultChan

	res.Send(w)

	// Measure elapsed milliseconds and send to metrics.
	elapsed := time.Since(start)
	metrics.Increment("SuccessfulRequests")
	metrics.Add("TotalRequestTimeMS", elapsed.Milliseconds())
}

// handleJSONTransportRequest handles any incoming JSON transport requests. First it validates the JWT, then it verifies if
// the json data for the particular user exists, and finally it sends the data through the transport request map.
func handleJSONTransportRequest(serverevent httputils.ServerRequest, transportRequestMap map[mandelboxtypes.MandelboxID]chan *JSONTransportRequest, transportMapLock *sync.Mutex) {
	// Receive the value of the `json_transport request`
	req := serverevent.(*JSONTransportRequest)

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
		transportRequestMap[req.MandelboxID] = make(chan *JSONTransportRequest, 1)
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
	transportRequestMap map[mandelboxtypes.MandelboxID]chan *JSONTransportRequest, transportMapLock *sync.Mutex) chan *JSONTransportRequest {
	// Acquire lock on transport requests map
	transportMapLock.Lock()
	defer transportMapLock.Unlock()

	req := transportRequestMap[mandelboxID]

	if req == nil {
		transportRequestMap[mandelboxID] = make(chan *JSONTransportRequest, 1)
	}

	return transportRequestMap[mandelboxID]
}

func getAppName(mandelboxSubscription subscriptions.Mandelbox, transportRequestMap map[mandelboxtypes.MandelboxID]chan *JSONTransportRequest,
	transportMapLock *sync.Mutex) (*JSONTransportRequest, mandelboxtypes.AppName) {

	var AppName mandelboxtypes.AppName
	var req *JSONTransportRequest

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

// Function to authenticate an incoming request. Splitting this into a separate
// function has the following advantages:
// 1. We factor out duplicated functionality among all the endpoints that need
// authentication.
// 2. Doing so allows us not to unmarshal the `jwt_access_token` field, and
// therefore prevents needing to pass it to client code in other packages that
// don't need to understand our authentication mechanism or read the secret
// key. We could alternatively do this by creating two separate types of
// structs containing the data required for the endpoint --- one without the
// auth secret, and one with it --- but this requires duplication of struct
// definitions and writing code to copy values from one struct to another.
// 3. If we get a bad, unauthenticated request we can minimize the amount of
// processsing power we devote to it. This is useful for being resistant to
// Denial-of-Service attacks.
func authenticateRequest(w http.ResponseWriter, r *http.Request, s httputils.ServerRequest, authorizeAsBackend bool) (err error) {
	// Get the raw map of the request, and unmarshal into
	// the corresponding struct
	rawmap, err := httputils.ParseRequest(w, r, s)
	if err != nil {
		return utils.MakeError("Error while parsing request. Err: %v", err)
	}

	if authorizeAsBackend {
		var requestAuthSecret string

		err = func() error {
			if value, ok := rawmap["jwt_access_token"]; ok {
				return json.Unmarshal(*value, &requestAuthSecret)
			}
			return utils.MakeError("Request body had no \"jwt_access_token\" field.")
		}()

		if err != nil {
			http.Error(w, "Unauthorized", http.StatusUnauthorized)
			return utils.MakeError("Error getting jwt_access_token from JSON body sent on %s to URL %s: %s", r.Host, r.URL, err)
		}

		// Actually verify authentication. We check that the access token sent is a valid JWT signed by Auth0.
		// Parses a raw access token string, verifies the token's signature, ensures that it is valid at the current moment in time.
		claims, err := auth.ParseToken(requestAuthSecret)
		if err != nil {
			http.Error(w, "Unauthorized", http.StatusUnauthorized)
			return utils.MakeError("Received an unpermissioned backend request on %s to URL %s. Error: %s", r.Host, r.URL, err)
		}

		if err := auth.Verify(claims); err != nil {
			http.Error(w, "Unauthorized", http.StatusUnauthorized)
			return utils.MakeError("Received an unpermissioned backend request on %s to URL %s. Error: %s", r.Host, r.URL, err)
		}
	}

	return nil
}

// StartHTTPServer returns a channel of events from the webserver as its first return value
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

	// Create a custom HTTP Request Multiplexer
	mux := http.NewServeMux()
	mux.Handle("/", http.NotFoundHandler())
	mux.HandleFunc("/json_transport", createHandler(processJSONDataRequest))

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
