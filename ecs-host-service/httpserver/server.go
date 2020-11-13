package httpserver

import (
	"io"
	"net/http"
	"os/exec"

	logger "github.com/fractal/ecs-host-service/fractallogger"
)

// We listen on the port "HOST" converted to a telephone number
const (
	portToListen       = ":4678"
	FractalPrivatePath = "/fractalprivate/"
	certPath           = FractalPrivatePath + "cert.pem"
	privatekeyPath     = FractalPrivatePath + "key.pem"
)

func initializeTLS() error {
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
		"-keyout",
		privatekeyPath,
		"-out",
		certPath,
	)
	output, err := cmd.CombinedOutput()
	if err != nil {
		return logger.MakeError("Unable to create x509 private key/certificate pair. Error: %v, Command output: %s", err, output)
	}

	logger.Info("Successfully created TLS certificate/private key pair.")
	return nil
}

type ServerEvent interface {
	HandleEvent() error
}

// The first return value is a channel of events from the webserver
func StartHTTPSServer() (<-chan ServerEvent, error) {
	logger.Info("Setting up HTTP server.")

	err := initializeTLS()
	if err != nil {
		return nil, logger.MakeError("Error starting HTTP Server: %v", err)
	}

	// Buffer up to 100 events so we don't block
	events := make(chan ServerEvent, 100)

	helloHandler := func(w http.ResponseWriter, req *http.Request) {
		logger.Infof("Got a connection!")
		io.WriteString(w, "Hello, world!\n")
	}

	http.HandleFunc("/", helloHandler)
	go func() {
		// TODO: defer things correctly so that a panic here is actually caught and resolved
		logger.Panicf("HTTP Server Error: %v", http.ListenAndServeTLS("0.0.0.0"+portToListen, certPath, privatekeyPath, nil))
	}()

	return events, nil
}
