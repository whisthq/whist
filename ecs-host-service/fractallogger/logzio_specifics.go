package fractallogger

import (
	"github.com/logzio/logzio-go"
	"time"
)

// Variable containing our logz.io shipping token --- filled in by linker if built on CI
var logzioShippingToken string

// We use a pointer of this type so we can check if it is nil in our logging
// functions, and therefore always call them safely.
var logzioTransport *logzio.LogzioSender

func initializeLogzIO() (*logzio.LogzioSender, error) {
	if UsingProdLogging() {
		Info("Setting up logz.io integration.")
	} else {
		Info("Not setting up logz.io integration.")
		return nil, nil
	}

	if logzioShippingToken == "" {
		return nil, MakeError("Error initializing logz.io integration: logzioShippingToken is uninitialized")
	}

	sender, err := logzio.New(
		logzioShippingToken,
		logzio.SetUrl("https://listener.logz.io:8071"),
		logzio.SetDrainDuration(time.Second*5),
		logzio.SetCheckDiskSpace(false),
	)
	if err != nil {
		return nil, MakeError("Error initializing logz.io integration: %s", err)
	}

	return sender, nil
}

func StopAndDrainLogzio() {
	if logzioTransport != nil {
		logzioTransport.Stop()
	}
}
