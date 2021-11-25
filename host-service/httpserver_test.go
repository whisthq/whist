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

// TestSendRequestResultErr checks if an error result is handled properly
func TestSendRequestResultErr(t *testing.T) {
	reqResult := requestResult{
		Err: utils.MakeError("test error"),
	}
	res := httptest.NewRecorder()

	// A 406 error should arise
	reqResult.send(*res)

	if res.Result().StatusCode != http.StatusNotAcceptable {
		t.Fatalf("error sending request results with error. Expected status code %v, got %v", http.StatusNotAcceptable, res.Result().StatusCode)
	}
}

// TestSendRequestResult tests if a valid request resolves successfully
func TestSendRequestResult(t *testing.T) {
	reqResult := requestResult{
		Result: "test result",
	}
	res := httptest.NewRecorder()

	// A 200 status code should be set
	reqResult.send(*res)

	if res.Result().StatusCode != http.StatusOK {
		t.Fatalf("error sending a valid request results. Expected status code %v, got %v", http.StatusOK, res.Result().StatusCode)
	}
}

// TestProcessJSONDataRequestWrongType checks if the wrong request type fails silently
func TestProcessJSONDataRequestWrongType(t *testing.T) {
	res := httptest.NewRecorder()
	req := httptest.NewRequest(httptest.MethodPost)
	testChan := make(chan ServerRequest)

	// With the wrong request type processJSONDataRequest will fail quietly
	processJSONDataRequest(res, req, testChan)

	select {
		case result := <-testChan:
			t.Fatalf("error processing json data request with wrong request type. Expected test server request chan to be empty")
		default:
	}
}

// TestProcessJSONDataRequestEmptyBody checks if an empty body fails silently
func TestProcessJSONDataRequestEmptyBody(t *testing.T) {
	res := httptest.NewRecorder()
	req := httptest.NewRequest(httptest.MethodPut)
	testChan := make(chan ServerRequest)

	// With an empty request body processJSONDataRequest will fail quietly (will not be able to unmarshal)
	processJSONDataRequest(res, req, testChan)

	select {
		case result := <-testChan:
			t.Fatalf("error processing json data request with empty. Expected test server request chan to be empty")
		default:
	}
}

// TestHandleJSONTransportRequest checks if valid fields will successfully send json transport request
func TestHandleJSONTransportRequest(t *testing.T) {
	testJSONTransportRequest := JSONTransportRequest{
		ConfigEncryptionToken: "testToken1234",
		JwtAccessToken:        "test_jwt_token",
		MandelboxID:           mandelboxtypes.MandelboxID(utils.PlaceholderTestUUID()),
		JSONData:              "test_json_data",
		resultChan:            make(chan requestResult),
	}

	testmux := &sync.Mutex{}
	testTransportRequestMap := make(map[mandelboxtypes.MandelboxID]chan *JSONTransportRequest)

	handleJSONTransportRequest(&testJSONTransportRequest, testTransportRequestMap, testmux)

	select {
		case result := <-testTransportRequestMap[req.MandelboxID]:
			return
		default:
			t.Fatalf("error handling json transport requests. Expected a request from the request chan")
	}
}

// TestGetJSONTransportRequestChannel checks if JSON transport request is created for the mandelboxID
func TestGetJSONTransportRequestChannel(t *testing.T) {
	mandelboxID := mandelboxtypes.MandelboxID(utils.PlaceholderTestUUID())
	testmux := &sync.Mutex{}
	testTransportRequestMap := make(map[mandelboxtypes.MandelboxID]chan *JSONTransportRequest)

	// Should create a new JSONTransportRequest chan
	testJsonChan := getJSONTransportRequestChannel(mandelboxID, testTransportRequestMap, testmux)

	if _, ok := interface{}(testJsonChan).(chan *JSONTransportRequest); !ok {
		t.Fatalf("error getting json transport request channel")
	}
}


// TestGetAppNameEmpty will check if default AppName is browser/chrome
func TestGetAppNameEmpty(t *testing.T) {
	mandelboxID := mandelboxtypes.MandelboxID(utils.PlaceholderTestUUID())
	testmux := &sync.Mutex{}
	testTransportRequestMap := make(map[mandelboxtypes.MandelboxID]chan *JSONTransportRequest)
	
	// Should default name to browser/chrome
	req, appName := getAppName(mandelboxID, testTransportRequestMap, testmux)
	
	if appName != mandelboxtypes.AppName("browsers/chrome") {
		t.Fatalf("error getting app name. Expected %v, got %v", mandelboxtypes.AppName("browsers/chrome"), appName)
	}

}

// TestGetAppName will set appName to json request app name
func TestGetAppName(t *testing.T) {generateTestJSONTransportRequest
	testJSONTransportRequest := JSONTransportRequest{
		AppName:		mandelboxtypes.AppName("test_app_name"),
	}

	mandelboxID := mandelboxtypes.MandelboxID(utils.PlaceholderTestUUID())
	testmux := &sync.Mutex{}
	testTransportRequestMap := make(map[mandelboxtypes.MandelboxID]chan *JSONTransportRequest)
	
	// Assign JSONTRansportRequest
	testTransportRequestMap[MandelboxID] = make(chan *JSONTransportRequest, 1)
	testTransportRequestMap[MandelboxID] <- testJSONTransportRequest

	// Should be set to test_app_name
	req, appName := getAppName(mandelboxID, testTransportRequestMap, testmux)
	
	if appName != testJSONTransportRequest.AppName {
		t.Fatalf("error getting app name. Expected %v, got %v", testJSONTransportRequest.AppName, appName)
	}
}

// TestVerifyRequestWrongType will create a request method that does not match the expected method
func TestVerifyRequestWrongType(t *testing.T) {
	res := httptest.NewRecorder()
	req := httptest.NewRequest(httptest.MethodPost)
	
	// Verify request type will catch request with wrong method and return an error
	if err := verifyRequestType(res, req, http.PUT); err == nil {
		t.Fatal("error verifying request type when the request method does not match. Expected an error, got nil")
	}

	if res.Result().StatusCode != http.StatusBadRequest {
		t.Fatalf("error verifying request type when the request method does not match. Expected status code %v, got %v", http.StatusBadRequest, res.Result().StatusCode)
	}
}

// TestVerifyRequestTypeNilRequest checks if nil requests are handled properly
func TestVerifyRequestTypeNilRequest(t *testing.T) {
	res := httptest.NewRecorder()

	// Verify request type will catch nil request and return an error
	if err := verifyRequestType(res, nil, httptest.MethodPut); err == nil {
		t.Fatal("error verifying request type when request is nil. Expected an error, got nil")
	}

	if res.Result().StatusCode != http.StatusBadRequest {
		t.Fatalf("error verifying request type when request is nil. Expected status code %v, got %v", http.StatusBadRequest, res.Result().StatusCode)
	}
	
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
