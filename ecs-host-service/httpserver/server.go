package httpserver // import "github.com/fractal/fractal/ecs-host-service/httpserver"

import (
	"context"
	"encoding/json"
	"io"
	"net/http"
	"os/exec"
	"sync"
	"time"

	"github.com/fractal/fractal/ecs-host-service/auth"
	"github.com/fractal/fractal/ecs-host-service/fractalcontainer/fctypes"
	"github.com/fractal/fractal/ecs-host-service/fractalcontainer/portbindings"
	logger "github.com/fractal/fractal/ecs-host-service/fractallogger"
	utils "github.com/fractal/fractal/ecs-host-service/utils"
)

// Constants for use in setting up the HTTPS server
const (
	// Our HTTPS server listens on the port "HOST" converted to a telephone
	// number.
	PortToListen   uint16 = 4678
	certPath       string = utils.FractalPrivateDir + "cert.pem"
	privatekeyPath string = utils.FractalPrivateDir + "key.pem"
)

func init() {
	portbindings.Reserve(PortToListen, portbindings.TransportProtocolTCP)
}

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

// SetContainerStartValuesRequest defines the (unauthenticated) start values
// endpoint (currently called by the webserver, soon to be removed altogether).
type SetContainerStartValuesRequest struct {
	HostPort             int                          `json:"host_port"`              // Port on the host to whose container the start values correspond
	DPI                  int                          `json:"dpi"`                    // DPI to set for the container
	UserID               fctypes.UserID               `json:"user_id"`                // User ID of the container user
	ClientAppAccessToken fctypes.ClientAppAccessToken `json:"client_app_auth_secret"` // User access token for client app verification
	ContainerARN         string                       `json:"container_ARN"`          // AWS ID of the container
	resultChan           chan requestResult           // Channel to pass the request result between goroutines
}

// ReturnResult is called to pass the result of a request back to the HTTP
// request handler.
func (s *SetContainerStartValuesRequest) ReturnResult(result interface{}, err error) {
	s.resultChan <- requestResult{result, err}
}

// createResultChan is called to create the Go channel to pass the request
// result back to the HTTP request handler via ReturnResult.
func (s *SetContainerStartValuesRequest) createResultChan() {
	if s.resultChan == nil {
		s.resultChan = make(chan requestResult)
	}
}

// Process an HTTP request for setting the start values of a container, to be handled in ecs-host-service.go
func processSetContainerStartValuesRequest(w http.ResponseWriter, r *http.Request, queue chan<- ServerRequest) {
	// Verify that it is an PUT request
	if verifyRequestType(w, r, http.MethodPut) != nil {
		return
	}

	// Verify authorization and unmarshal into the right object type
	var reqdata SetContainerStartValuesRequest
	if err := authenticateAndParseRequest(w, r, &reqdata, true); err != nil {
		logger.Errorf("Error authenticating and parsing %T: %s", reqdata, err)
		return
	}

	// Send request to queue, then wait for result
	queue <- &reqdata
	res := <-reqdata.resultChan

	res.send(w)
}

// SetConfigEncryptionTokenRequest defines the (unauthenticated) set config
// encryption token endpoint (currently called by the client application, soon
// to be removed altogether).
type SetConfigEncryptionTokenRequest struct {
	HostPort              int                           `json:"host_port"`               // Port on the host to whose container this user corresponds
	UserID                fctypes.UserID                `json:"user_id"`                 // User to whom token belongs
	ConfigEncryptionToken fctypes.ConfigEncryptionToken `json:"config_encryption_token"` // User-specific private encryption token
	ClientAppAccessToken  fctypes.ClientAppAccessToken  `json:"client_app_auth_secret"`  // User access token for client app verification
	JwtAccessToken        auth.RawJWT                   `json:"jwt_access_token"`        // User's JWT access token
	resultChan            chan requestResult            // Channel to pass the request result between goroutines
}

// ReturnResult is called to pass the result of a request back to the HTTP
// request handler.
func (s *SetConfigEncryptionTokenRequest) ReturnResult(result interface{}, err error) {
	s.resultChan <- requestResult{result, err}
}

// createResultChan is called to create the Go channel to pass the request
// result back to the HTTP request handler via ReturnResult.
func (s *SetConfigEncryptionTokenRequest) createResultChan() {
	if s.resultChan == nil {
		s.resultChan = make(chan requestResult)
	}
}

// Process an HTTP request for setting the start values of a container, to be handled in ecs-host-service.go
func processSetConfigEncryptionTokenRequest(w http.ResponseWriter, r *http.Request, queue chan<- ServerRequest) {
	// Verify that it is an PUT request
	if verifyRequestType(w, r, http.MethodPut) != nil {
		return
	}

	// Verify authorization and unmarshal into the right object type
	var reqdata SetConfigEncryptionTokenRequest
	if err := authenticateAndParseRequest(w, r, &reqdata, false); err != nil {
		logger.Errorf("Error authenticating and parsing %T: %s", reqdata, err)
		return
	}

	// Send request to queue, then wait for result
	queue <- &reqdata
	res := <-reqdata.resultChan

	res.send(w)
}

// SpinUpMandelboxRequest defines the (unauthenticated) `spin_up_mandelbox`
// endpoint. For now, this endpoint is only exposed in the `localdev`
// environment, and is used by `run_local_container_image.sh`. This endpoint
// returns the Docker ID of the container. Eventually, as we move off ECS, this
// endpoint will become the canonical way to start containers.
type SpinUpMandelboxRequest struct {
	AppImage              string                        `json:"app_image"`               // The image to spin up
	DPI                   int                           `json:"dpi"`                     // DPI to set for the container
	UserID                fctypes.UserID                `json:"user_id"`                 // User ID of the container user
	ConfigEncryptionToken fctypes.ConfigEncryptionToken `json:"config_encryption_token"` // User-specific private encryption token
	JwtAccessToken        auth.RawJWT                   `json:"jwt_access_token"`        // User's JWT access token
	resultChan            chan requestResult            // Channel to pass the request result between goroutines
}

// SpinUpMandelboxRequestResult defines the data returned by the
// `spin_up_mandelbox` endpoint.
type SpinUpMandelboxRequestResult struct {
	HostPortForTCP32262 uint16            `json:"port_32262"`
	HostPortForUDP32263 uint16            `json:"port_32263"`
	HostPortForTCP32273 uint16            `json:"port_32273"`
	AesKey              string            `json:"aes_key"`
	FractalID           fctypes.FractalID `json:"fractal_id"`
}

// ReturnResult is called to pass the result of a request back to the HTTP
// request handler.
func (s *SpinUpMandelboxRequest) ReturnResult(result interface{}, err error) {
	s.resultChan <- requestResult{result, err}
}

// createResultChan is called to create the Go channel to pass the request
// result back to the HTTP request handler via ReturnResult.
func (s *SpinUpMandelboxRequest) createResultChan() {
	if s.resultChan == nil {
		s.resultChan = make(chan requestResult)
	}
}

// Process an HTTP request to spin up a container, to be handled in ecs-host-service.go
func processSpinUpMandelboxRequest(w http.ResponseWriter, r *http.Request, queue chan<- ServerRequest) {
	// Verify that it is an PUT request
	if verifyRequestType(w, r, http.MethodPut) != nil {
		return
	}

	// Verify authorization and unmarshal into the right object type
	var reqdata SpinUpMandelboxRequest
	if err := authenticateAndParseRequest(w, r, &reqdata, false); err != nil {
		logger.Errorf("Error authenticating and parsing %T: %s", reqdata, err)
		return
	}

	// Send request to queue, then wait for result
	queue <- &reqdata
	res := <-reqdata.resultChan

	res.send(w)
}

// DrainAndShutdownRequest defines the (unauthenticated) drain_and_shutdown
// endpoint, called by the webserver as part of scaling down an instance.
type DrainAndShutdownRequest struct {
	resultChan chan requestResult // Channel to pass the request result between goroutines
}

// ReturnResult is called to pass the result of a request back to the HTTP
// request handler.
func (s *DrainAndShutdownRequest) ReturnResult(result interface{}, err error) {
	s.resultChan <- requestResult{result, err}
}

// createResultChan is called to create the Go channel to pass the request
// result back to the HTTP request handler via ReturnResult.
func (s *DrainAndShutdownRequest) createResultChan() {
	if s.resultChan == nil {
		s.resultChan = make(chan requestResult)
	}
}

// Process an HTTP request for setting the start values of a container, to be handled in ecs-host-service.go.
func processDrainAndShutdownRequest(w http.ResponseWriter, r *http.Request, queue chan<- ServerRequest) {
	// Verify that it is an POST request
	if verifyRequestType(w, r, http.MethodPost) != nil {
		return
	}

	// Verify authorization and unmarshal into the right object type
	var reqdata DrainAndShutdownRequest
	if err := authenticateAndParseRequest(w, r, &reqdata, true); err != nil {
		logger.Errorf("Error authenticating and parsing %T: %s", reqdata, err)
		return
	}

	// Send request to queue, then wait for result
	queue <- &reqdata
	res := <-reqdata.resultChan

	res.send(w)
}

// Helper functions

// Function to verify the type (method) of a request
func verifyRequestType(w http.ResponseWriter, r *http.Request, method string) error {
	if r.Method != method {
		err := utils.MakeError("Received a request on %s to URL %s of type %s, but it should have been type %s", r.Host, r.URL, r.Method, method)
		logger.Error(err)

		http.Error(w, utils.Sprintf("Bad request type. Expected %s, got %s", method, r.Method), http.StatusBadRequest)

		return err
	}
	return nil
}

// Function to get the body of a result, authenticate it, and unmarshal it into
// a golang struct. Splitting this into a separate function has the following
// advantages:
// 1. We factor out duplicated functionality among all the endpoints that need
// authentication.
// 2. Doing so allows us not to unmarshal the `auth_secret` field, and
// therefore prevents needing to pass it to client code in other packages that
// don't need to understand our authentication mechanism or read the secret
// key. We could alternatively do this by creating two separate types of
// structs containing the data required for the endpoint --- one without the
// auth secret, and one with it --- but this requires duplication of struct
// definitions and writing code to copy values from one struct to another.
// 3. If we get a bad, unauthenticated request we can minimize the amount of
// processsing power we devote to it. This is useful for being resistant to
// Denial-of-Service attacks.
func authenticateAndParseRequest(w http.ResponseWriter, r *http.Request, s ServerRequest, authenticate bool) (err error) {
	// Get body of request
	body, err := io.ReadAll(r.Body)
	if err != nil {
		http.Error(w, "Malformed body", http.StatusBadRequest)
		return utils.MakeError("Error getting body from request on %s to URL %s: %s", r.Host, r.URL, err)
	}

	// TODO: rename these auth_secrets to something more accurate, like `jwt`s or something.

	// Extract only the auth_secret field from a raw JSON unmarshalling that
	// delays as much decoding as possible
	var rawmap map[string]*json.RawMessage
	err = json.Unmarshal(body, &rawmap)
	if err != nil {
		http.Error(w, "Malformed body", http.StatusBadRequest)
		return utils.MakeError("Error raw-unmarshalling JSON body sent on %s to URL %s: %s", r.Host, r.URL, err)
	}

	if authenticate {
		var requestAuthSecret auth.RawJWT
		err = func() error {
			if value, ok := rawmap["auth_secret"]; ok {
				return json.Unmarshal(*value, &requestAuthSecret)
			}
			return utils.MakeError("Request body had no \"auth_secret\" field.")
		}()
		if err != nil {
			http.Error(w, "Unauthorized", http.StatusUnauthorized)
			return utils.MakeError("Error getting auth_secret from JSON body sent on %s to URL %s: %s", r.Host, r.URL, err)
		}

		// Actually verify authentication. We check that the access token sent is
		// a valid JWT signed by Auth0 and that it has the "backend" scope.
		claims, err := auth.Verify(requestAuthSecret)
		isPermissioned := auth.HasScope(claims, "backend")
		if err != nil || !isPermissioned {
			http.Error(w, "Unauthorized", http.StatusUnauthorized)
			return utils.MakeError("Received an unpermissioned backend request on %s to URL %s", r.Host, r.URL)
		}
	}

	// Now, actually do the unmarshalling into the right object type
	err = json.Unmarshal(body, s)
	if err != nil {
		http.Error(w, "Malformed body", http.StatusBadRequest)
		return utils.MakeError("Could not fully unmarshal the body of a request sent on %s to URL %s: %s", r.Host, r.URL, err)
	}

	// Set up the result channel
	s.createResultChan()

	return nil
}

// Start returns a channel of events from the webserver as its first return value
func Start(globalCtx context.Context, globalCancel context.CancelFunc, goroutineTracker *sync.WaitGroup) (<-chan ServerRequest, error) {
	logger.Info("Setting up HTTP server.")

	err := initializeTLS()
	if err != nil {
		return nil, utils.MakeError("Error starting HTTP Server: %v", err)
	}

	// Buffer up to 100 events so we don't block. This should be mostly
	// unnecessary, but we want to be able to handle a burst without blocking.
	events := make(chan ServerRequest, 100)

	createHandler := func(f func(http.ResponseWriter, *http.Request, chan<- ServerRequest)) func(http.ResponseWriter, *http.Request) {
		return func(w http.ResponseWriter, r *http.Request) {
			f(w, r, events)
		}
	}

	// Create a custom HTTP Request Multiplexer
	mux := http.NewServeMux()
	mux.Handle("/", http.NotFoundHandler())
	mux.HandleFunc("/set_container_start_values", createHandler(processSetContainerStartValuesRequest))
	mux.HandleFunc("/set_config_encryption_token", createHandler(processSetConfigEncryptionTokenRequest))
	mux.HandleFunc("/spin_up_mandelbox", createHandler(processSpinUpMandelboxRequest))
	mux.HandleFunc("/drain_and_shutdown", createHandler(processDrainAndShutdownRequest))

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
		shutdownCtx, shutdownCancel := context.WithTimeout(globalCtx, 5*time.Second)
		defer shutdownCancel()

		err := server.Shutdown(shutdownCtx)
		if err != nil {
			logger.Errorf("Shut down httpserver with error %s", err)
		} else {
			logger.Info("Gracefully shut down httpserver.")
		}
	}()

	return events, nil
}

// Creates a TLS certificate/private key pair for secure communication with the Fractal webserver
func initializeTLS() error {
	// Create a self-signed passwordless certificate
	// https://unix.stackexchange.com/questions/104171/create-ssl-certificate-non-interactively
	cmd := exec.Command(
		"/usr/bin/openssl",
		"req",
		"-new",
		"-newkey",
		"rsa:4096",
		"-days",
		"365",
		"-nodes",
		"-x509",
		"-subj",
		"/C=US/ST=./L=./O=./CN=.",
		"-addext", "subjectAltName=IP:127.0.0.1",
		"-keyout",
		privatekeyPath,
		"-out",
		certPath,
	)
	output, err := cmd.CombinedOutput()
	if err != nil {
		return utils.MakeError("Unable to create x509 private key/certificate pair. Error: %v, Command output: %s", err, output)
	}

	logger.Info("Successfully created TLS certificate/private key pair. Certificate path: %s", certPath)
	return nil
}
