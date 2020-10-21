package fractalwebserver

import (
	"bytes"
	"encoding/json"
	"io/ioutil"
	"math/rand"
	"net/http"
	"time"

	// We use this package instead of the standard library log so that we never
	// forget to send a message via sentry.  For the same reason, we make sure
	// not to import the fmt package either, instead separating required
	// functionality in this imported package as well.
	logger "github.com/fractalcomputers/ecs-host-service/fractallogger"
)

const stagingHost = "https://staging-webserver.tryfractal.com"
const productionHost = "https://main-webserver.tryfractal.com"

const authEndpoint = "/host_service/auth"
const heartbeatEndpoint = "/host_service/heartbeat"

// TODO: change the webserver to use the production or staging host based on an
// environment variable
const webserverHost = stagingHost

type handshakeRequest struct {
	InstanceID string
}

type handshakeResponse struct {
	AuthToken string
}

type heartbeatRequest struct {
	AuthToken        string
	Timestamp        string
	HeartbeatNumber  uint64
	InstanceID       string
	InstanceType     string
	TotalRAMinKB     string
	FreeRAMinKB      string
	AvailRAMinKB     string
	IsDyingHeartbeat bool
}

var authToken string
var numBeats uint64 = 0
var httpClient = http.Client{
	Timeout: 10 * time.Second,
}

func InitializeHeartbeat() error {
	resp, err := handshake()
	if err != nil {
		return logger.MakeError("Error handshaking with webserver: %v", err)
	}

	authToken = resp.AuthToken

	go heartbeatGoroutine()

	return nil
}

// Instead of running exactly every minute, we choose a random time in the
// range [55, 65] seconds to prevent waves of hosts repeatedly crowding the
// webserver. Note also that we don't have to do any error handling here
// because sendHeartbeat() does not return or panic.
func heartbeatGoroutine() {
	for {
		sendHeartbeat(false)
		sleepTime := 65000 - rand.Intn(10001)
		time.Sleep(time.Duration(sleepTime) * time.Millisecond)
	}
}

func SendGracefulShutdownNotice() {
	sendHeartbeat(true)
}

// Talk to the auth endpoint for the host service startup (to authenticate all
// future requests). We currently send the EC2 instance ID in the request as a
// (weak) layer of defense against unauthenticated/spoofed handshakes. We
// expect back a JSON response containing a field called "AuthToken"
func handshake() (handshakeResponse, error) {
	var resp handshakeResponse

	instanceID, err := logger.GetAwsInstanceId()
	if err != nil {
		return resp, logger.MakeError("handshake(): Couldn't get AWS instanceID. Error: %v", err)
	}

	requestURL := webserverHost + authEndpoint
	requestBody, err := json.Marshal(handshakeRequest{instanceID})
	if err != nil {
		return resp, logger.MakeError("handshake(): Could not marshal the handshakeRequest object. Error: %v", err)
	}

	logger.Infof("handshake(): Sending a POST request with body %s to URL %s", requestBody, requestURL)
	httpResp, err := httpClient.Post(requestURL, "application/json", bytes.NewReader(requestBody))
	if err != nil {
		return resp, logger.MakeError("handshake(): Got back an error from the webserver at URL %s. Error:  %v", requestURL, err)
	}

	body, err := ioutil.ReadAll(httpResp.Body)
	if err != nil {
		return resp, logger.MakeError("handshake():: Unable to read body of response from webserver. Error: %v", err)
	}

	logger.Infof("handshake(): got response code: %v", httpResp.StatusCode)
	logger.Infof("handshake(): got response: %s", body)

	err = json.Unmarshal(body, &resp)
	if err != nil {
		return resp, logger.MakeError("handshake():: Unable to unmarshal JSON response from the webserver!. Response: %s Error: %s", body, err)
	}
	return resp, nil
}

// We send a heartbeat. Note that the heartbeat number 0 is the initialization
// message of the host service to the webserver.  This function intentionally
// does not panic or return an error/value. Instead, we let the webserver deal
// with malformed heartbeats by potentially choosing to mark the instance as
// draining. This also simplifies our logging/error handling since we can just
// ignore errors in the heartbeat goroutine.
func sendHeartbeat(isDying bool) {
	// Prepare the body

	// We ignore errors in these function calls because errors will just get
	// passed on as an empty string or nil. It's not worth terminating the
	// instance over a malformed heartbeat --- we can let the webserver decide if
	// we want to mark the instance as draining.
	instanceID, _ := logger.GetAwsInstanceId()
	instanceType, _ := logger.GetAwsInstanceType()
	totalRAM, _ := logger.GetTotalMemoryInKB()
	freeRAM, _ := logger.GetFreeMemoryInKB()
	availRAM, _ := logger.GetAvailableMemoryInKB()

	requestURL := webserverHost + heartbeatEndpoint
	requestBody, err := json.Marshal(heartbeatRequest{
		AuthToken:        authToken,
		Timestamp:        logger.Sprintf("%s", time.Now()),
		HeartbeatNumber:  numBeats,
		InstanceID:       instanceID,
		InstanceType:     instanceType,
		TotalRAMinKB:     totalRAM,
		FreeRAMinKB:      freeRAM,
		AvailRAMinKB:     availRAM,
		IsDyingHeartbeat: isDying,
	})
	if err != nil {
		logger.Errorf("Couldn't marshal requestBody into JSON. Error: %v", err)
	}

	logger.Infof("Sending a heartbeat with body %s to URL %s", requestBody, requestURL)
	_, err = httpClient.Post(requestURL, "application/json", bytes.NewReader(requestBody))
	if err != nil {
		logger.Errorf("Error sending heartbeat: %s", err)
	}

	numBeats += 1
}
