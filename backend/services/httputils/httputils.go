package httputils

import (
	"bytes"
	"encoding/json"
	"io"
	"io/ioutil"
	"net/http"
	"os/exec"
	"strings"

	"github.com/whisthq/whist/backend/services/host-service/auth"
	"github.com/whisthq/whist/backend/services/metadata"
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

// Send is called to send an HTTP response
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
		logger.Errorf("error marshalling a %v HTTP Response body: %s", status, err)
	}
	_, _ = w.Write(buf)
}

// Helper functions

// GetAccessToken is a helper function that extracts the access token
// from the request "Authorization" header. If it fails, fallback to
// extracting the token from the request's body.
func GetAccessToken(r *http.Request) (string, error) {
	if metadata.IsLocalEnv() {
		return "", nil
	}

	authorization := r.Header.Get("Authorization")
	bearer := strings.Split(authorization, "Bearer ")

	var (
		accessToken string
		bodyMap     map[string]interface{}
	)
	if len(bearer) <= 1 || bearer[1] == "" || bearer[1] == "undefined" {
		logger.Warningf("Bearer token is empty. Trying to parse token from request body.")

		// Read request body and replace for subsequent reads
		body, err := ioutil.ReadAll(r.Body)
		if err != nil {
			return "", utils.MakeError("failed to read request body: %s", err)
		}
		r.Body.Close()
		r.Body = io.NopCloser(bytes.NewBuffer(body))

		// Here we unmarshal into a simple map because we are only interested
		// in the access token.
		err = json.Unmarshal(body, &bodyMap)
		if err != nil {
			return "", utils.MakeError("failed to umarshal request body: %s", err)
		}

		val, ok := bodyMap["jwt_access_token"]
		if !ok {
			return "", utils.MakeError("did not find jwt_access_token field in request body.")
		}

		// TODO: We've decided to supersede the client-app with Chromium on June 29, 2022. It no
		// longer gets deployed. In ~1 month, remove this logic to only obtain the token from
		// the "jwt_access_token" key. -- If the date is past what's above and this code is still
		// here, simply make a PR to remove it.
		accessToken = val.(string)

	} else {
		accessToken = bearer[1]
	}

	return accessToken, nil
}

// AuthenticateRequest will verify that the access token is valid
// and will parse the request body and try to unmarshal into a
// `ServerRequest` type.
func AuthenticateRequest(w http.ResponseWriter, r *http.Request, s ServerRequest) (*auth.WhistClaims, error) {
	accessToken, err := GetAccessToken(r)
	if err != nil {
		return nil, err
	}

	var claims *auth.WhistClaims
	// Skip token validation if running on local environment
	if !metadata.IsLocalEnv() {
		claims, err = auth.ParseToken(accessToken)
		if err != nil {
			http.Error(w, "Unauthorized", http.StatusUnauthorized)
			return nil, utils.MakeError("received an unpermissioned backend request on %s to URL %s: %s", r.Host, r.URL, err)
		}

		if err := auth.Verify(claims); err != nil {
			http.Error(w, "Unauthorized", http.StatusUnauthorized)
			return nil, utils.MakeError("received an unpermissioned backend request on %s to URL %s: %s", r.Host, r.URL, err)
		}
	}

	_, err = ParseRequest(w, r, s)
	if err != nil {
		return nil, utils.MakeError("Error while parsing request. Err: %v", err)
	}

	return claims, nil
}

// ParseRequest will split the request body, unmarshal into a raw JSON map, and then unmarshal
// the remaining fields into the struct `s`. We unmarshal the raw JSON map and the rest of the
// body so that we don't expose the authorization fields to code that does not handle authentication.
func ParseRequest(w http.ResponseWriter, r *http.Request, s ServerRequest) (map[string]*json.RawMessage, error) {
	// Get body of request
	body, err := io.ReadAll(r.Body)
	if err != nil {
		http.Error(w, "Malformed body", http.StatusBadRequest)
		return nil, utils.MakeError("error getting body from request on %s to URL %s: %s", r.Host, r.URL, err)
	}

	// Extract only the jwt_access_token field from a raw JSON unmarshalling that
	// delays as much decoding as possible
	var rawmap map[string]*json.RawMessage
	err = json.Unmarshal(body, &rawmap)
	if err != nil {
		http.Error(w, "Malformed body", http.StatusBadRequest)
		return nil, utils.MakeError("error raw-unmarshalling JSON body sent on %s to URL %s: %s", r.Host, r.URL, err)
	}

	// Now, actually do the unmarshalling into the right object type
	err = json.Unmarshal(body, s)
	if err != nil {
		http.Error(w, "Malformed body", http.StatusBadRequest)
		return nil, utils.MakeError("could not fully unmarshal the body of a request sent on %s to URL %s: %s", r.Host, r.URL, err)
	}

	// Set up the result channel
	s.CreateResultChan()

	return rawmap, nil
}

// Function to verify the type (method) of a request
func VerifyRequestType(w http.ResponseWriter, r *http.Request, method string) error {
	if r == nil {
		err := utils.MakeError("received a nil request expecting to be type %s", method)
		logger.Error(err)

		http.Error(w, utils.Sprintf("Bad request. Expected %s, got nil", method), http.StatusBadRequest)

		return err
	}

	if r.Method != method {
		err := utils.MakeError("received a request on %s to URL %s of type %s, but it should have been type %s", r.Host, r.URL, r.Method, method)
		logger.Error(err)

		http.Error(w, utils.Sprintf("Bad request type. Expected %s, got %s", method, r.Method), http.StatusBadRequest)

		return err
	}
	return nil
}

// EnableCORS is a middleware that sets the Access control header to accept requests from all origins.
func EnableCORS(f func(http.ResponseWriter, *http.Request)) func(http.ResponseWriter, *http.Request) {
	return http.HandlerFunc(func(rw http.ResponseWriter, r *http.Request) {
		rw.Header().Set("Access-Control-Allow-Origin", "*")
		rw.Header().Set("Access-Control-Allow-Headers", "Origin Accept Content-Type X-Requested-With")
		rw.Header().Set("Access-Control-Allow-Methods", "POST PUT OPTIONS")

		if r.Method == http.MethodOptions {
			http.Error(rw, "No Content", http.StatusNoContent)
			return
		}

		f(rw, r)
	})
}

// Creates a TLS certificate/private key pair for secure communication with the Whist frontend
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

	logger.Infof("Successfully created TLS certificate/private key pair. Certificate path: %s", certPath)
	return nil
}
