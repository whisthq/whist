package main

import (
	"bytes"
	"context"
	"crypto/tls"
	"encoding/json"
	"errors"
	"io"
	"net/http"
	"net/http/httptest"
	"reflect"
	"sync"
	"testing"
	"testing/iotest"
	"time"

	mandelbox "github.com/whisthq/whist/backend/services/host-service/mandelbox"
	"github.com/whisthq/whist/backend/services/host-service/mandelbox/configutils"
	"github.com/whisthq/whist/backend/services/httputils"
	"github.com/whisthq/whist/backend/services/subscriptions"
	mandelboxtypes "github.com/whisthq/whist/backend/services/types"
	"github.com/whisthq/whist/backend/services/utils"
)

type JSONTransportResult struct {
	Result httputils.JSONTransportRequestResult `json:"result"`
}

// TestSpinUpHandler calls processSpinUpMandelboxRequest and checks to see if
// request data is successfully passed into the processing queue.
func TestSpinUpHandler(t *testing.T) {

	validBookmarks := `{"roots": {"bookmark_bar": {"children": [{"date_added": "13280861983875934","name": "whist.com","url": "http://whist.com"}],"date_added": "13280861983875934","name": "Bookmark Bar","type": "folder"}}}`
	bookmarks, err := configutils.UnmarshalBookmarks(mandelboxtypes.Bookmarks(validBookmarks))
	if err != nil {
		t.Fatalf("UnmarshalBookmarks returned an unexpected error: %v", err)
	}

	browserData := mandelbox.BrowserData{
		CookiesJSON: `[{"creation_utc": 13280861983875934, "host_key": "whist.com"}]`,
		Bookmarks:   &bookmarks,
		Extensions:  "not_real_extension_id,not_real_second_extension_id",
	}

	marshalledBrowserData, err := json.Marshal(browserData)
	if err != nil {
		t.Fatalf("could not marshal browser data: %v", err)
	}

	deflatedBrowserData, err := configutils.GzipDeflateString(string(marshalledBrowserData))
	if err != nil {
		t.Fatalf("could not deflate browser data: %v", err)
	}

	deflatedJSONData, err := configutils.GzipDeflateString(string("test_json_data"))
	if err != nil {
		t.Fatalf("could not deflate JSON data: %v", err)
	}

	testJSONTransportRequest := httputils.JSONTransportRequest{
		ConfigEncryptionToken: "test_token",
		JwtAccessToken:        "test_jwt_token",
		MandelboxID:           mandelboxtypes.MandelboxID(utils.PlaceholderTestUUID()),
		JSONData:              mandelboxtypes.JSONData(deflatedJSONData),
		BrowserData:           mandelboxtypes.BrowserData(deflatedBrowserData),
		ResultChan:            make(chan httputils.RequestResult),
	}

	testServerQueue := make(chan httputils.ServerRequest)
	receivedRequest := make(chan httputils.ServerRequest)

	res := httptest.NewRecorder()
	httpRequest, err := generateTestJSONTransportRequest(testJSONTransportRequest)
	if err != nil {
		t.Fatalf("error creating json transport request: %v", err)
	}

	testResult := httputils.JSONTransportRequestResult{
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

	var gotRequestMap httputils.JSONTransportRequest
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
		{"JSONData", string(testJSONTransportRequest.JSONData), string(gotRequestMap.JSONData)},
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

// TestHttpServerIntegration spins up an HTTP server and checks that requests are processed correctly
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

	validBookmarks := `{"roots": {"bookmark_bar": {"children": [{"date_added": "13280861983875934","name": "whist.com","url": "http://whist.com"}],"date_added": "13280861983875934","name": "Bookmark Bar","type": "folder"}}}`
	bookmarks, err := configutils.UnmarshalBookmarks(mandelboxtypes.Bookmarks(validBookmarks))
	if err != nil {
		t.Fatalf("UnmarshalBookmarks returned an unexpected error: %v", err)
	}

	browserData := mandelbox.BrowserData{
		CookiesJSON: `[{"creation_utc": 13280861983875934, "host_key": "whist.com"}]`,
		Bookmarks:   &bookmarks,
		Extensions:  "",
	}

	marshalledBrowserData, err := json.Marshal(browserData)
	if err != nil {
		t.Fatalf("could not marshal browser data: %v", err)
	}

	deflatedBrowserData, err := configutils.GzipDeflateString(string(marshalledBrowserData))
	if err != nil {
		t.Fatalf("could not deflate browser data: %v", err)
	}

	deflatedJSONData, err := configutils.GzipDeflateString(string("test_json_data"))
	if err != nil {
		t.Fatalf("could not deflate JSON data: %v", err)
	}

	testJSONTransportRequest := httputils.JSONTransportRequest{
		ConfigEncryptionToken: "test_token",
		JwtAccessToken:        "test_jwt_token",
		MandelboxID:           mandelboxtypes.MandelboxID(utils.PlaceholderTestUUID()),
		JSONData:              mandelboxtypes.JSONData(deflatedJSONData),
		BrowserData:           mandelboxtypes.BrowserData(deflatedBrowserData),
		ResultChan:            make(chan httputils.RequestResult),
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

	testResult := httputils.JSONTransportRequestResult{
		HostPortForTCP32262: 32262,
		HostPortForUDP32263: 32263,
		HostPortForTCP32273: 32273,
		AesKey:              "aesKey",
	}
	gotRequestChan := make(chan httputils.ServerRequest)

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

	var gotRequestMap httputils.JSONTransportRequest
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
		{"JSONData", string(testJSONTransportRequest.JSONData), string(gotRequestMap.JSONData)},
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
	reqResult := httputils.RequestResult{
		Err: utils.MakeError("test error"),
	}
	res := httptest.NewRecorder()

	// A 406 error should arise
	reqResult.Send(res)

	if res.Result().StatusCode != http.StatusNotAcceptable {
		t.Fatalf("error sending request results with error. Expected status code %v, got %v", http.StatusNotAcceptable, res.Result().StatusCode)
	}
}

// TestSendRequestResult tests if a valid request resolves successfully
func TestSendRequestResult(t *testing.T) {
	reqResult := httputils.RequestResult{
		Result: "test result",
	}
	res := httptest.NewRecorder()
	io.WriteString(res, "test content")

	// A 200 status code should be set
	reqResult.Send(res)

	if res.Result().StatusCode != http.StatusOK {
		t.Fatalf("error sending a valid request results. Expected status code %v, got %v", http.StatusOK, res.Result().StatusCode)
	}
}

// TestProcessJSONDataRequestWrongType checks if the wrong request will result in the request not being added to the queue
func TestProcessJSONDataRequestWrongType(t *testing.T) {
	res := httptest.NewRecorder()
	req := httptest.NewRequest(http.MethodPost, "https://localhost", nil)
	testChan := make(chan httputils.ServerRequest)

	// processJSONDataRequest will return being trying to authenticate and parse request given the wrong request type
	processJSONDataRequest(res, req, testChan)

	select {
	case <-testChan:
		t.Fatalf("error processing json data request with wrong request type. Expected test server request chan to be empty")
	default:
	}
}

// TestProcessJSONDataRequestEmptyBody checks if an empty body will result in the request not being added to the queue
func TestProcessJSONDataRequestEmptyBody(t *testing.T) {
	res := httptest.NewRecorder()
	req := httptest.NewRequest(http.MethodPut, "https://localhost", nil)
	testChan := make(chan httputils.ServerRequest)

	// processJSONDataRequest will fail to parse request with an empty request body (will not be able to unmarshal)
	processJSONDataRequest(res, req, testChan)

	select {
	case <-testChan:
		t.Fatalf("error processing json data request with empty. Expected test server request chan to be empty")
	default:
	}
}

// TestHandleJSONTransportRequest checks if valid fields will successfully send JSON transport request
func TestHandleJSONTransportRequest(t *testing.T) {
	deflatedJSONData, err := configutils.GzipDeflateString(string("test_json_data"))
	if err != nil {
		t.Fatalf("could not deflate JSON data: %v", err)
	}
	testJSONTransportRequest := httputils.JSONTransportRequest{
		ConfigEncryptionToken: "testToken1234",
		JwtAccessToken:        "test_jwt_token",
		MandelboxID:           mandelboxtypes.MandelboxID(utils.PlaceholderTestUUID()),
		JSONData:              mandelboxtypes.JSONData(deflatedJSONData),
		ResultChan:            make(chan httputils.RequestResult),
	}

	testmux := &sync.Mutex{}
	testTransportRequestMap := make(map[mandelboxtypes.MandelboxID]chan *httputils.JSONTransportRequest)

	handleJSONTransportRequest(&testJSONTransportRequest, testTransportRequestMap, testmux)

	select {
	case <-testTransportRequestMap[testJSONTransportRequest.MandelboxID]:
		return
	default:
		t.Fatalf("error handling json transport requests. Expected a request from the request chan")
	}
}

// TestGetJSONTransportRequestChannel checks if JSON transport request is created for the mandelboxID
func TestGetJSONTransportRequestChannel(t *testing.T) {
	mandelboxID := mandelboxtypes.MandelboxID(utils.PlaceholderTestUUID())
	testmux := &sync.Mutex{}
	testTransportRequestMap := make(map[mandelboxtypes.MandelboxID]chan *httputils.JSONTransportRequest)

	// getJSONTransportRequestChannel will create a new JSON transport request channel with the mandelboxID
	testJsonChan := getJSONTransportRequestChannel(mandelboxID, testTransportRequestMap, testmux)

	if _, ok := interface{}(testJsonChan).(chan *httputils.JSONTransportRequest); !ok {
		t.Fatalf("error getting json transport request channel")
	}
}

// TestGetAppNameEmpty will check if default AppName is browser/chrome
func TestGetAppNameEmpty(t *testing.T) {
	mandelboxID := mandelboxtypes.MandelboxID(utils.PlaceholderTestUUID())
	testmux := &sync.Mutex{}
	testTransportRequestMap := make(map[mandelboxtypes.MandelboxID]chan *httputils.JSONTransportRequest)

	// Assign JSONTRansportRequest
	testTransportRequestMap[mandelboxID] = make(chan *httputils.JSONTransportRequest, 1)
	testTransportRequestMap[mandelboxID] <- &httputils.JSONTransportRequest{}

	testMandelboxSubscription := subscriptions.Mandelbox{
		ID:         mandelboxID,
		App:        "",
		InstanceID: "test-instance-id",
		UserID:     "test-user-id",
		SessionID:  "1234567890",
		Status:     "ALLOCATED",
		CreatedAt:  time.Now(),
		UpdatedAt:  time.Now(),
	}
	// Should default name to browsers/chrome
	_, appName := getAppName(testMandelboxSubscription, testTransportRequestMap, testmux)

	if appName != mandelboxtypes.AppName("browsers/chrome") {
		t.Fatalf("error getting app name. Expected %v, got %v", mandelboxtypes.AppName("browsers/chrome"), appName)
	}
}

// TestGetAppNameNoRequest will time out and return nil request
func TestGetAppNameNoRequest(t *testing.T) {
	mandelboxID := mandelboxtypes.MandelboxID(utils.PlaceholderTestUUID())
	testmux := &sync.Mutex{}
	testTransportRequestMap := make(map[mandelboxtypes.MandelboxID]chan *httputils.JSONTransportRequest)

	testMandelboxSubscription := subscriptions.Mandelbox{
		ID:         mandelboxID,
		App:        "",
		InstanceID: "test-instance-id",
		UserID:     "test-user-id",
		SessionID:  "1234567890",
		Status:     "ALLOCATED",
		CreatedAt:  time.Now(),
		UpdatedAt:  time.Now(),
	}

	// Will take 1 minute to resolve and return nil request
	req, _ := getAppName(testMandelboxSubscription, testTransportRequestMap, testmux)

	if req != nil {
		t.Fatalf("error getting app name with no transport request. Expected nil, got %v", req)
	}
}

// TestGetAppName will set appName to JSON request app name
func TestGetAppName(t *testing.T) {
	var appNames = []string{"browsers/chrome", "browsers/brave", "browsers/test"}

	for _, appName := range appNames {
		testJSONTransportRequest := httputils.JSONTransportRequest{
			AppName: mandelboxtypes.AppName(appName),
		}

		mandelboxID := mandelboxtypes.MandelboxID(utils.PlaceholderTestUUID())
		testmux := &sync.Mutex{}
		testTransportRequestMap := make(map[mandelboxtypes.MandelboxID]chan *httputils.JSONTransportRequest)

		// Assign JSONTRansportRequest
		testTransportRequestMap[mandelboxID] = make(chan *httputils.JSONTransportRequest, 1)
		testTransportRequestMap[mandelboxID] <- &testJSONTransportRequest

		testMandelboxSubscription := subscriptions.Mandelbox{
			ID:         mandelboxID,
			App:        appName,
			InstanceID: "test-instance-id",
			UserID:     "test-user-id",
			SessionID:  "1234567890",
			Status:     "ALLOCATED",
			CreatedAt:  time.Now(),
			UpdatedAt:  time.Now(),
		}

		// getAppName should get an appName that matches AppName in testJSONTransportRequest
		_, mandelboxAppName := getAppName(testMandelboxSubscription, testTransportRequestMap, testmux)

		if mandelboxAppName != testJSONTransportRequest.AppName {
			t.Fatalf("error getting app name. Expected %v, got %v", testJSONTransportRequest.AppName, appName)
		}
	}
}

// TestVerifyRequestWrongType will create a request method that does not match the expected method
func TestVerifyRequestWrongType(t *testing.T) {
	res := httptest.NewRecorder()
	req := httptest.NewRequest(http.MethodPost, "https://localhost", nil)

	// verifyRequestType will catch request with wrong method and return an error
	if err := httputils.VerifyRequestType(res, req, http.MethodPut); err == nil {
		t.Fatal("error verifying request type when the request method does not match. Expected an error, got nil")
	}

	if res.Result().StatusCode != http.StatusBadRequest {
		t.Fatalf("error verifying request type when the request method does not match. Expected status code %v, got %v", http.StatusBadRequest, res.Result().StatusCode)
	}
}

// TestVerifyRequestTypeNilRequest checks if nil requests are handled properly
func TestVerifyRequestTypeNilRequest(t *testing.T) {
	res := httptest.NewRecorder()

	// verifyRequestType will catch nil request and return an error
	if err := httputils.VerifyRequestType(res, nil, http.MethodPut); err == nil {
		t.Fatal("error verifying request type when request is nil. Expected an error, got nil")
	}

	if res.Result().StatusCode != http.StatusBadRequest {
		t.Fatalf("error verifying request type when request is nil. Expected status code %v, got %v", http.StatusBadRequest, res.Result().StatusCode)
	}
}

// TestAuthenticateAndParseRequestReadAllErr checks if body read errors are handled properly
func TestAuthenticateAndParseRequestReadAllErr(t *testing.T) {
	res := httptest.NewRecorder()
	req := httptest.NewRequest(http.MethodPut, "https://localhost", iotest.ErrReader(errors.New("test error")))
	testJSONTransportRequest := httputils.JSONTransportRequest{
		ResultChan: make(chan httputils.RequestResult),
	}

	// authenticateAndParseRequest will get an error trying to read request body and will cause an error
	_, err := httputils.AuthenticateRequest(res, req, &testJSONTransportRequest)

	if err == nil {
		t.Fatalf("error authenticating and parsing request when real all fails. Expected err, got nil")
	}

	if res.Result().StatusCode != http.StatusBadRequest {
		t.Fatalf("error authenticating and parsing request when real all fails. Expected status code %v, got %v", http.StatusBadRequest, res.Result().StatusCode)
	}
}

// TestAuthenticateAndParseRequestEmptyBody checks if an empty body will error successfully
func TestAuthenticateAndParseRequestEmptyBody(t *testing.T) {
	res := httptest.NewRecorder()
	req := httptest.NewRequest(http.MethodPut, "https://localhost", bytes.NewReader([]byte{}))
	testJSONTransportRequest := httputils.JSONTransportRequest{
		ResultChan: make(chan httputils.RequestResult),
	}

	// authenticateAndParseRequest will be unable to unmarshal an empty body and will cause an error
	_, err := httputils.AuthenticateRequest(res, req, &testJSONTransportRequest)

	if err == nil {
		t.Fatalf("error authenticating and parsing request with empty body. Expected err, got nil")
	}

	if res.Result().StatusCode != http.StatusBadRequest {
		t.Fatalf("error authenticating and parsing request with empty body. Expected status code %v, got %v", http.StatusBadRequest, res.Result().StatusCode)
	}
}

// generateTestJSONTransportRequest takes a request body and creates an
// HTTP PUT request for /json_transport
func generateTestJSONTransportRequest(requestBody httputils.JSONTransportRequest) (*http.Request, error) {
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
