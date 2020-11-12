package httpserver

import (
	"io"
	"net/http"

	logger "github.com/fractal/ecs-host-service/fractallogger"
)

// We listen on the port "HOST" converted to a telephone number
const portToListen = ":4678"

type ServerEvent interface {
	HandleEvent() error
}

// The first return value is a channel of events from the webserver
func StartHttpServer() (<-chan ServerEvent, error) {
	logger.Info("Setting up webserver.")

	events := make(chan ServerEvent)

	helloHandler := func(w http.ResponseWriter, req *http.Request) {
		logger.Infof("Got a connection!")
		io.WriteString(w, "Hello, world!\n")
	}

	http.HandleFunc("/", helloHandler)
	http.ListenAndServe("0.0.0.0"+portToListen, nil)

	return events, nil
}
