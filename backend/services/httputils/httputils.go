package httputils

import (
	"encoding/json"
	"io"
	"net/http"
	"os/exec"

	"github.com/whisthq/whist/backend/services/host-service/metrics"
	"github.com/whisthq/whist/backend/services/utils"
	logger "github.com/whisthq/whist/backend/services/whistlogger"
)

// A ServerRequest represents a request from the server --- it is exported so
// that we can implement the top-level event handlers in parent packages. They
// simply return the result and any error message via ReturnResult.
type ServerRequest interface {
	ReturnResult(result interface{}, err error)
	CreateResultChan()
}

// A RequestResult represents the result of a request that was successfully
// authenticated, parsed, and processed by the consumer.
type RequestResult struct {
	Result interface{} `json:"-"`
	Err    error       `json:"error"`
}

// send is called to send an HTTP response
func (r RequestResult) Send(w http.ResponseWriter) {
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
		metrics.Increment("ErrorRate")
	}
	_, _ = w.Write(buf)
}

// Request types

// Mandelbox assign request
type MandelboxAssignRequest struct {
	Regions    []string           `json:"regions"`
	CommitHash string             `json:"client_commit_hash"`
	SessionID  int64              `json:"session_id"`
	UserEmail  string             `json:"user_email"`
	ResultChan chan RequestResult // Channel to pass the request result between goroutines
}

type MandelboxAssignRequestResult struct {
}

// ReturnResult is called to pass the result of a request back to the HTTP
// request handler.
func (s *MandelboxAssignRequest) ReturnResult(result interface{}, err error) {
	s.ResultChan <- RequestResult{
		Result: result,
		Err:    err,
	}
}

// createResultChan is called to create the Go channel to pass the request
// result back to the HTTP request handler via ReturnResult.
func (s *MandelboxAssignRequest) CreateResultChan() {
	if s.ResultChan == nil {
		s.ResultChan = make(chan RequestResult)
	}
}

// Helper functions

// ParseRequest will split the request body, unmarshal into a raw JSON map, and then unmarshal
// the remaining fields into the struct `s`. We unmarshal the raw JSON map and the rest of the
// body so that we don't expose the authorization fields to code that does not handle authentication.
func ParseRequest(w http.ResponseWriter, r *http.Request, s ServerRequest) (map[string]*json.RawMessage, error) {
	// Get body of request
	body, err := io.ReadAll(r.Body)
	if err != nil {
		http.Error(w, "Malformed body", http.StatusBadRequest)
		return nil, utils.MakeError("Error getting body from request on %s to URL %s: %s", r.Host, r.URL, err)
	}

	// Extract only the jwt_access_token field from a raw JSON unmarshalling that
	// delays as much decoding as possible
	var rawmap map[string]*json.RawMessage
	err = json.Unmarshal(body, &rawmap)
	if err != nil {
		http.Error(w, "Malformed body", http.StatusBadRequest)
		return nil, utils.MakeError("Error raw-unmarshalling JSON body sent on %s to URL %s: %s", r.Host, r.URL, err)
	}

	// Now, actually do the unmarshalling into the right object type
	err = json.Unmarshal(body, s)
	if err != nil {
		http.Error(w, "Malformed body", http.StatusBadRequest)
		return nil, utils.MakeError("Could not fully unmarshal the body of a request sent on %s to URL %s: %s", r.Host, r.URL, err)
	}

	// Set up the result channel
	s.CreateResultChan()

	return rawmap, nil
}

// Function to verify the type (method) of a request
func VerifyRequestType(w http.ResponseWriter, r *http.Request, method string) error {
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

// Creates a TLS certificate/private key pair for secure communication with the Whist webserver
func InitializeTLS(privatekeyPath string, certPath string) error {
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
