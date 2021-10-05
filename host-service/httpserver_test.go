package main

import (
	"bytes"
	"context"
	"crypto/tls"
	"encoding/json"
	"io"
	"net/http"
	"net/http/httptest"
	"reflect"
	"sync"
	"testing"
	"time"

	"github.com/fractal/fractal/host-service/auth"
	"github.com/fractal/fractal/host-service/utils"
	"github.com/golang-jwt/jwt/v4"
)

type SpinUpResult struct {
	Result SpinUpMandelboxRequestResult `json:"result"`
}

type DrainResult struct {
	Result string `json:"result"`
}

// TestUnauthenticatedRequest calls authenticateAndParseRequest with improperly authenticated requests
// and checks if the correct errors are returned.
func TestUnauthenticatedRequest(t *testing.T) {
	testCases := make(map[string]map[string]string)

	testCases["No auth field"] = map[string]string{
		"test": "test",
	}

	testCases["Improper auth format"] = map[string]string{
		"auth_secret": "some gibberish",
		"test":        "test",
	}

	// Sign a random JWT that should fail verification
	token := jwt.NewWithClaims(jwt.SigningMethodHS256, jwt.StandardClaims{})
	tokenString, err := token.SignedString([]byte("test"))
	if err != nil {
		t.Fatalf("error signing test token: %v", err)
	}

	testCases["Invalid JWT"] = map[string]string{
		"auth_secret": tokenString,
		"test":        "test",
	}

	for testDesc, testBody := range testCases {
		t.Logf("Running test for: %s", testDesc)
		t.Run(testDesc, func(t *testing.T) {
			jsonData, err := json.Marshal(testBody)
			if err != nil {
				t.Fatalf("error marshalling json: %v", err)
			}

			request, err := http.NewRequest(http.MethodPost, "/drain_and_shutdown", bytes.NewBuffer(jsonData))
			if err != nil {
				t.Fatalf("error creating post request: %v", err)
			}

			res := httptest.NewRecorder()
			dummyServerRequest := DrainAndShutdownRequest{}
			authenticateAndParseRequest(res, request, &dummyServerRequest, true)

			got := res.Code
			want := http.StatusUnauthorized

			if got != want {
				t.Errorf("got code %d, want %d", got, want)
			}
		})
	}
}

// TestSpinUpHandler calls processSpinUpMandelboxRequest and checks to see if
// request data is successfully passed into the processing queue.
func TestSpinUpHandler(t *testing.T) {
	testRequest := map[string]interface{}{
		"app_name":                "test_app",
		"config_encryption_token": "test_token",
		"jwt_access_token":        "test_jwt_token",
		"mandelbox_id":            "test_mandelbox",
		"session_id":              float64(1234),
	}

	testServerQueue := make(chan ServerRequest)
	receivedRequest := make(chan ServerRequest)

	res := httptest.NewRecorder()
	httpRequest, err := generateTestSpinUpRequest(testRequest)
	if err != nil {
		t.Fatalf("error creating spin up request: %v", err)
	}

	testResult := SpinUpMandelboxRequestResult{
		HostPortForTCP32262: 32262,
		HostPortForUDP32263: 32263,
		HostPortForTCP32273: 32273,
		AesKey:              "aesKey",
	}

	// Goroutine to receive request from processing queue and simulate successful spinup
	go func() {
		request := <-testServerQueue
		request.ReturnResult(testResult, nil)
		receivedRequest <- request
	}()

	processSpinUpMandelboxRequest(res, httpRequest, testServerQueue)
	gotRequest := <-receivedRequest

	var gotResult SpinUpResult
	resBody, err := io.ReadAll(res.Result().Body)
	if err != nil {
		t.Fatalf("error reading result body: %v", resBody)
	}

	err = json.Unmarshal(resBody, &gotResult)
	if err != nil {
		t.Fatalf("error unmarshalling json: %v", err)
	}

	// Check that we are successfully receiving requests on the server channel
	jsonGotRequest, err := json.Marshal(gotRequest)
	if err != nil {
		t.Fatalf("error marshalling json: %v", err)
	}

	var gotRequestMap map[string]interface{}
	err = json.Unmarshal(jsonGotRequest, &gotRequestMap)
	if err != nil {
		t.Fatalf("error unmarshalling json: %v", err)
	}

	for key, value := range testRequest {
		if gotRequestMap[key] != value {
			t.Errorf("expected request key %s to be %v of type %T, got %v of type %T", key, value, value, gotRequestMap[key], gotRequestMap[key])
		}
	}

	// Check that we are successfully receiving replies on the result channel
	if !reflect.DeepEqual(testResult, gotResult.Result) {
		t.Errorf("expected result %v, got %v", testResult, gotResult)
	}
}

// TestDrainAndShutdownHandler calls processDrainAndShutdownRequest and checks to see if
// request data is successfully passed into the processing queue.
func TestDrainAndShutdownHandler(t *testing.T) {
	testServerQueue := make(chan ServerRequest)
	receivedRequest := make(chan ServerRequest)

	res := httptest.NewRecorder()
	httpRequest, err := generateTestDrainRequest()
	if err != nil {
		t.Fatalf("error creating drain request: %v", err)
	}

	testResult := "request successful"

	// Goroutine to receive request from processing queue and simulate successful drain
	go func() {
		request := <-testServerQueue
		request.ReturnResult(testResult, nil)
		receivedRequest <- request
	}()

	// Mock auth function to not check auth and revert it after test
	origFunc := authenticateAndParseRequest
	authenticateAndParseRequest = mockAuthenticateRequest
	processDrainAndShutdownRequest(res, httpRequest, testServerQueue)
	<-receivedRequest
	authenticateAndParseRequest = origFunc

	var gotResult DrainResult
	resBody, err := io.ReadAll(res.Result().Body)
	if err != nil {
		t.Fatalf("error reading result body: %v", resBody)
	}

	err = json.Unmarshal(resBody, &gotResult)
	if err != nil {
		t.Fatalf("error unmarshalling json: %v", err)
	}

	// Check that we are successfully receiving replies on the result channel
	if testResult != gotResult.Result {
		t.Errorf("expected result %v, got %v", testResult, gotResult)
	}
}

// TestHttpServerIntegration spins up a webserver and checks that requests are processed correctly
func TestHttpServerIntegration(t *testing.T) {
	globalCtx, globalCancel := context.WithCancel(context.Background())
	goroutineTracker := sync.WaitGroup{}

	initializeFilesystem(globalCancel)
	defer uninitializeFilesystem()

	httpServerEvents, err := StartHTTPServer(globalCtx, globalCancel, &goroutineTracker)
	if err != nil && err.Error() != "Shut down httpserver with error context canceled" {
		t.Fatalf("error starting http server: %v", err)
	}

	// Wait for server startup
	time.Sleep(5 * time.Second)

	testRequest := map[string]interface{}{
		"app_name":                "test_app",
		"config_encryption_token": "test_token",
		"jwt_access_token":        "test_jwt_token",
		"mandelbox_id":            "test_mandelbox",
		"session_id":              float64(1234),
	}
	req, err := generateTestSpinUpRequest(testRequest)
	if err != nil {
		t.Fatalf("error creating spin up request: %v", err)
	}

	// Disable certificate verification on client for testing
	tlsConfig := &tls.Config{
		InsecureSkipVerify: true,
	}

	tr := &http.Transport{
		TLSClientConfig: tlsConfig,
	}

	client := &http.Client{
		Transport: tr,
	}

	testResult := SpinUpMandelboxRequestResult{
		HostPortForTCP32262: 32262,
		HostPortForUDP32263: 32263,
		HostPortForTCP32273: 32273,
		AesKey:              "aesKey",
	}
	gotRequestChan := make(chan ServerRequest)

	// Receive requests from the HTTP server and "process" them in a
	// separate goroutine by returning a predetermined result
	go func() {
		receivedRequest := <-httpServerEvents
		receivedRequest.ReturnResult(testResult, nil)
		gotRequestChan <- receivedRequest
	}()

	res, err := client.Do(req)
	if err != nil {
		t.Errorf("error calling server: %v", err)
	}

	// Check that we are successfully receiving requests on the server channel
	gotRequest := <-gotRequestChan
	jsonGotRequest, err := json.Marshal(gotRequest)
	if err != nil {
		t.Fatalf("error marshalling json: %v", err)
	}

	var gotRequestMap map[string]interface{}
	err = json.Unmarshal(jsonGotRequest, &gotRequestMap)
	if err != nil {
		t.Fatalf("error unmarshalling json: %v", err)
	}

	for key, value := range testRequest {
		if gotRequestMap[key] != value {
			t.Errorf("expected request key %s to be %v, got %v", key, value, gotRequestMap[key])
		}
	}

	// Check that we are successfully receiving replies on the result channel
	var gotResult SpinUpResult

	body, err := io.ReadAll(res.Body)
	if err != nil {
		t.Fatalf("error reading response body: %v", err)
	}

	err = json.Unmarshal(body, &gotResult)
	if err != nil {
		t.Fatalf("error unmarshalling json: %v", err)
	}

	if !reflect.DeepEqual(testResult, gotResult.Result) {
		t.Errorf("expected result %v, got %v", testResult, gotResult)
	}

	globalCancel()
	goroutineTracker.Wait()
	t.Log("server goroutine ended")
}

// generateTestSpinUpRequest takes a request body and creates an
// HTTP PUT request for /spin_up_mandelbox
func generateTestSpinUpRequest(requestBody map[string]interface{}) (*http.Request, error) {
	jsonData, err := json.Marshal(requestBody)
	if err != nil {
		return nil, utils.MakeError("error marshalling json: %v", err)
	}

	httpRequest, err := http.NewRequest(http.MethodPut,
		utils.Sprintf("https://localhost:%d/spin_up_mandelbox", PortToListen),
		bytes.NewBuffer(jsonData),
	)
	if err != nil {
		return nil, utils.MakeError("error creating put request: %v", err)
	}

	return httpRequest, nil
}

// generateTestDrainRequest creates an HTTP POST request for /drain_and_shutdown
func generateTestDrainRequest() (*http.Request, error) {
	claims := auth.FractalClaims{
		Audience: auth.Audience{"test"},
		Scopes:   auth.Scopes{"backend"},
	}
	token := jwt.NewWithClaims(jwt.SigningMethodHS256, claims)
	tokenString, err := token.SignedString([]byte("test"))
	if err != nil {
		return nil, utils.MakeError("error signing test token: %v", err)
	}

	testRequest := map[string]interface{}{
		"auth_secret": tokenString,
	}
	jsonData, err := json.Marshal(testRequest)
	if err != nil {
		return nil, utils.MakeError("error marshalling json: %v", err)
	}

	httpRequest, err := http.NewRequest(http.MethodPost,
		utils.Sprintf("https://localhost:%d/drain_and_shutdown", PortToListen),
		bytes.NewBuffer(jsonData),
	)
	if err != nil {
		return nil, utils.MakeError("error creating post request: %v", err)
	}

	return httpRequest, nil
}

// mockAuthenticateRequest pretends to be the authenticateAndParseRequest function
// but skips JWT signature verification steps to allow for unit testing
// without real Auth0 tokens
func mockAuthenticateRequest(w http.ResponseWriter, r *http.Request, s ServerRequest, authorizeAsBackend bool) (err error) {
	// Get body of request
	body, err := io.ReadAll(r.Body)
	if err != nil {
		http.Error(w, "Malformed body", http.StatusBadRequest)
		return utils.MakeError("Error getting body from request on %s to URL %s: %s", r.Host, r.URL, err)
	}

	// Extract only the auth_secret field from a raw JSON unmarshalling that
	// delays as much decoding as possible
	var rawmap map[string]*json.RawMessage
	err = json.Unmarshal(body, &rawmap)
	if err != nil {
		http.Error(w, "Malformed body", http.StatusBadRequest)
		return utils.MakeError("Error raw-unmarshalling JSON body sent on %s to URL %s: %s", r.Host, r.URL, err)
	}

	// Skip the authentication steps for mock

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
