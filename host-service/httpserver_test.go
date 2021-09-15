package main

import (
	"bytes"
	"context"
	"crypto/rand"
	"crypto/rsa"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"net/http/httptest"
	"reflect"
	"sync"
	"testing"

	"github.com/fractal/fractal/host-service/auth"
	"github.com/golang-jwt/jwt"
)

// TestUnauthenticatedRequest calls authenticateAndParseRequest with improperly authenticated requests
// and checks if the correct errors are returned
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
	token := jwt.NewWithClaims(jwt.SigningMethodRS256, jwt.StandardClaims{})
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
		"session_id":              1234,
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

	var gotResult SpinUpMandelboxRequestResult
	err = json.Unmarshal(res.Body.Bytes(), &gotResult)
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
			t.Errorf("expected request key %s to be %v, got %v", key, value, gotRequestMap[key])
		}
	}

	// Check that we are successfully receiving replies on the result channel
	if !reflect.DeepEqual(testResult, gotResult) {
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

	processDrainAndShutdownRequest(res, httpRequest, testServerQueue)
	<-receivedRequest

	var gotResult string
	err = json.Unmarshal(res.Body.Bytes(), &gotResult)
	if err != nil {
		t.Fatalf("error unmarshalling json: %v", err)
	}

	// Check that we are successfully receiving replies on the result channel
	if !reflect.DeepEqual(testResult, gotResult) {
		t.Errorf("expected result %v, got %v", testResult, gotResult)
	}
}

// TestHttpServerIntegration spins up a webserver and checks that requests are processed correctly
func TestHttpServerIntegration(t *testing.T) {
	globalCtx, globalCancel := context.WithCancel(context.Background())
	goroutineTracker := sync.WaitGroup{}

	httpServerEvents, err := StartHTTPServer(globalCtx, globalCancel, &goroutineTracker)
	if err != nil {
		t.Fatalf("error starting http server: %v", err)
	}

	testRequest := map[string]interface{}{
		"app_name":                "test_app",
		"config_encryption_token": "test_token",
		"jwt_access_token":        "test_jwt_token",
		"mandelbox_id":            "test_mandelbox",
		"session_id":              1234,
	}
	req, err := generateTestSpinUpRequest(testRequest)
	if err != nil {
		t.Fatalf("error creating spin up request: %v", err)
	}

	client := &http.Client{}
	res, err := client.Do(req)
	if err != nil {
		t.Errorf("error calling server: %v", err)
	}

	receivedRequest := <-httpServerEvents

	testResult := SpinUpMandelboxRequestResult{
		HostPortForTCP32262: 32262,
		HostPortForUDP32263: 32263,
		HostPortForTCP32273: 32273,
		AesKey:              "aesKey",
	}
	receivedRequest.ReturnResult(testResult, nil)

	// Check that we are successfully receiving requests on the server channel
	jsonGotRequest, err := json.Marshal(receivedRequest)
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
	var gotResult SpinUpMandelboxRequestResult

	body, err := io.ReadAll(res.Body)
	if err != nil {
		t.Fatalf("error reading response body: %v", err)
	}

	err = json.Unmarshal(body, &gotResult)
	if err != nil {
		t.Fatalf("error unmarshalling json: %v", err)
	}

	if !reflect.DeepEqual(testResult, gotResult) {
		t.Errorf("expected result %v, got %v", testResult, gotResult)
	}

	globalCancel()
	goroutineTracker.Wait()
	t.Log("server goroutine ended")
}

// generateRsaKeyPair generates a random RSA key pair
func generateRsaKeyPair() (*rsa.PrivateKey, *rsa.PublicKey) {
	privkey, _ := rsa.GenerateKey(rand.Reader, 4096)
	return privkey, &privkey.PublicKey
}

// generateTestSpinUpRequest takes a request body and creates an
// HTTP PUT request for /spin_up_mandelbox
func generateTestSpinUpRequest(requestBody map[string]interface{}) (*http.Request, error) {
	jsonData, err := json.Marshal(requestBody)
	if err != nil {
		return nil, fmt.Errorf("error marshalling json: %v", err)
	}

	httpRequest, err := http.NewRequest(http.MethodPut,
		fmt.Sprintf("https://localhost:%d/spin_up_mandelbox", PortToListen),
		bytes.NewBuffer(jsonData),
	)
	if err != nil {
		return nil, fmt.Errorf("error creating put request: %v", err)
	}

	return httpRequest, nil
}

// generateTestDrainRequest creates an HTTP POST request for /drain_and_shutdown
func generateTestDrainRequest() (*http.Request, error) {
	claims := auth.FractalClaims{
		Audience: []string{"test"},
		Scopes:   []string{"backend"},
	}
	token := jwt.NewWithClaims(jwt.SigningMethodRS256, claims)

	privateKey, _ := generateRsaKeyPair()
	tokenString, err := token.SignedString(privateKey)
	if err != nil {
		return nil, fmt.Errorf("error signing test token: %v", err)
	}

	testRequest := map[string]interface{}{
		"auth_secret": tokenString,
	}
	jsonData, err := json.Marshal(testRequest)
	if err != nil {
		return nil, fmt.Errorf("error marshalling json: %v", err)
	}

	httpRequest, err := http.NewRequest(http.MethodPost,
		fmt.Sprintf("https://localhost:%d/drain_and_shutdown", PortToListen),
		bytes.NewBuffer(jsonData),
	)
	if err != nil {
		return nil, fmt.Errorf("error creating post request: %v", err)
	}

	return httpRequest, nil
}
