package fractalwebserver

import (
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
	instanceID string
}

type handshakeResponse struct {
	authToken string
}

func Initialize() {
	handshake()
}

// Talk to the auth endpoint for the host service startup (to authenticate all
// future requests). We currently send the EC2 instance ID in the request as a
// (weak) layer of defense against unauthenticated/spoofed handshakes. We
// expect back a JSON response containing a field called "authToken"
func handshake() (handshakeResponse, error) {
	var resp handshakeResponse

	instanceID, err := logger.GetAwsInstanceId()
	if err != nil {
		return resp, logger.MakeError("fractalwebserver::handshake(): Couldn't get AWS instanceID. Error: %v", err)
	}

	requestURL := webserverHost + authEndpoint + "/" + instanceID
	logger.Infof("fractalwebserver::handshake(): Sending a POST request with empty body to URL %s", requestURL)
	httpResp, err := http.Post(requestURL, "application/json", nil)
	if err != nil {
		return resp, logger.MakeError("fractalwebserver::handshake(): Got back an error from the webserver at URL %s. Error:  %v", requestURL, err)
	}

	body, err := ioutil.ReadAll(httpResp.Body)
	if err != nil {
		return resp, logger.MakeError("fractalwebserver::handshake():: Unable to read body of response from webserver. Error: %v", err)
	}

	logger.Infof("fractalwebserver::handshake(): got response code: %v", httpResp.StatusCode)
	logger.Infof("fractalwebserver::handshake(): got response: %s", body)

	return resp, nil
}

func sendGET() {

}

func sendPOST() {

}

func SendHeartbeatSendHeartbeat() {

}
