package fractalwebserver

import (
	"bytes"
	"encoding/json"
	"io/ioutil"
	"net/http"

	// We use this package instead of the standard library log so that we never
	// forget to send a message via sentry.  For the same reason, we make sure
	// not to import the fmt package either, instead separating required
	// functionality in this imported package as well.
	logger "github.com/fractalcomputers/ecs-host-service/fractallogger"
)

const stagingHost = "https://staging-webserver.tryfractal.com"
const productionHost = "https://main-webserver.tryfractal.com"

const authEndpoint = "/host-service/auth"
const heartbeatEndpoint = "/host-service/heartbeat"

// TODO: change the webserver to use the production or staging host based on an
// environment variable
const webserverHost = stagingHost

type handshakeRequest struct {
	InstanceID string
}

type handshakeResponse struct {
	AuthToken string
}

var authToken string

func Initialize() error {
	resp, err := handshake()
	if err != nil {
		return logger.MakeError("Error handshaking with webserver: %v", err)
	}

	authToken = resp.AuthToken

	return nil
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
	httpResp, err := http.Post(requestURL, "application/json", bytes.NewReader(requestBody))
	if err != nil {
		return resp, logger.MakeError("handshake(): Got back an error from the webserver at URL %s. Error:  %v", requestURL, err)
	}

	body, err := ioutil.ReadAll(httpResp.Body)
	if err != nil {
		return resp, logger.MakeError("handshake():: Unable to read body of response from webserver. Error: %v", err)
	}

	logger.Infof("handshake(): got response code: %v", httpResp.StatusCode)
	logger.Infof("handshake(): got response: %s", body)

	// TODO: get rid of this testing
	body, err = json.Marshal(handshakeResponse{"testauthtoken"})
	logger.Infof("testResponse: %s", body)

	err = json.Unmarshal(body, &resp)
	if err != nil {
		return resp, logger.MakeError("handshake():: Unable to unmarshal JSON response from the webserver!. Response: %s Error: %s", body, err)
	}
	return resp, nil
}

func sendGET() {

}

func sendPOST() {

}

func SendHeartbeatSendHeartbeat() {

}
