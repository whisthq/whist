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

	mandelboxtypes "github.com/fractal/fractal/host-service/mandelbox/types"
	"github.com/fractal/fractal/host-service/utils"
)

type JSONTransportResult struct {
	Result JSONTransportRequestResult `json:"result"`
}

// TestSpinUpHandler calls processSpinUpMandelboxRequest and checks to see if
// request data is successfully passed into the processing queue.
func TestSpinUpHandler(t *testing.T) {
	testJSONTransportRequest := JSONTransportRequest{
		ConfigEncryptionToken: "test_token",
		JwtAccessToken:        "test_jwt_token",
		MandelboxID:           mandelboxtypes.MandelboxID(utils.PlaceholderTestUUID()),
		JSONData:              "test_json_data",
		Cookies:               "[{'creation_utc': 13280861983875934, 'host_key': 'whist.com'}]",
		Bookmarks:			   "{ 'test_bookmark': '1'}",
		resultChan:            make(chan requestResult),
	}

	testServerQueue := make(chan ServerRequest)
	receivedRequest := make(chan ServerRequest)

	res := httptest.NewRecorder()
	httpRequest, err := generateTestJSONTransportRequest(testJSONTransportRequest)
	if err != nil {
		t.Fatalf("error creating json transport request: %v", err)
	}

	testResult := JSONTransportRequestResult{
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

	processJSONDataRequest(res, httpRequest, testServerQueue)
	gotRequest := <-receivedRequest

	var gotResult JSONTransportResult
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

	var gotRequestMap JSONTransportRequest
	err = json.Unmarshal(jsonGotRequest, &gotRequestMap)
	if err != nil {
		t.Fatalf("error unmarshalling json: %v", err)
	}

	testMap := []struct {
		key       string
		want, got string
	}{
		{"ConfigEncryptionToken", string(testJSONTransportRequest.ConfigEncryptionToken), string(gotRequestMap.ConfigEncryptionToken)},
		{"JWTAccessToken", testJSONTransportRequest.JwtAccessToken, gotRequestMap.JwtAccessToken},
		{"JSONData", testJSONTransportRequest.JSONData, gotRequestMap.JSONData},
	}

	for _, value := range testMap {
		if value.got != value.want {
			t.Errorf("expected request key %s to be %v, got %v", value.key, value.got, value.want)
		}
	}

	// Check that we are successfully receiving replies on the result channel
	if !reflect.DeepEqual(testResult, gotResult.Result) {
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

	testJSONTransportRequest := JSONTransportRequest{
		ConfigEncryptionToken: "test_token",
		JwtAccessToken:        "test_jwt_token",
		MandelboxID:           mandelboxtypes.MandelboxID(utils.PlaceholderTestUUID()),
		JSONData:              "test_json_data",
		Cookies:               "[{'creation_utc': 13280861983875934, 'host_key': 'whist.com'}]",
		Bookmarks:			   "{ 'test_bookmark': '1'}",
		resultChan:            make(chan requestResult),
	}
	req, err := generateTestJSONTransportRequest(testJSONTransportRequest)
	if err != nil {
		t.Fatalf("error creating json transport request: %v", err)
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

	testResult := JSONTransportRequestResult{
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

	var gotRequestMap JSONTransportRequest
	err = json.Unmarshal(jsonGotRequest, &gotRequestMap)
	if err != nil {
		t.Fatalf("error unmarshalling json: %v", err)
	}

	testMap := []struct {
		key       string
		want, got string
	}{
		{"ConfigEncryptionToken", string(testJSONTransportRequest.ConfigEncryptionToken), string(gotRequestMap.ConfigEncryptionToken)},
		{"JWTAccessToken", testJSONTransportRequest.JwtAccessToken, gotRequestMap.JwtAccessToken},
		{"JSONData", testJSONTransportRequest.JSONData, gotRequestMap.JSONData},
	}

	for _, value := range testMap {
		if value.got != value.want {
			t.Errorf("expected request key %s to be %v, got %v", value.key, value.got, value.want)
		}
	}

	// Check that we are successfully receiving replies on the result channel
	var gotResult JSONTransportResult

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

// generateTestJSONTransportRequest takes a request body and creates an
// HTTP PUT request for /json_transport
func generateTestJSONTransportRequest(requestBody JSONTransportRequest) (*http.Request, error) {
	jsonData, err := json.Marshal(requestBody)
	if err != nil {
		return nil, utils.MakeError("error marshalling json: %v", err)
	}

	httpRequest, err := http.NewRequest(http.MethodPut,
		utils.Sprintf("https://localhost:%d/json_transport", PortToListen),
		bytes.NewBuffer(jsonData),
	)
	if err != nil {
		return nil, utils.MakeError("error creating put request: %v", err)
	}

	return httpRequest, nil
}
