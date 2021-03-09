package fractallogger

import (
	"log"
	"time"

	"github.com/logzio/logzio-go"
)

// Variable containing our logz.io shipping token --- filled in by linker if built on CI
var logzioShippingToken string

// We use a pointer of this type so we can check if it is nil in our logging
// functions, and therefore always call them safely.
var logzioTransport *logzioSender

// We define a custom type to be able to override the Send() function with sane
// error handling.
type logzioSender logzio.LogzioSender

func (sender *logzioSender) Send(payload string) {
	err := (*logzio.LogzioSender)(sender).Send([]byte(payload))
	if err != nil {
		log.Printf(ColorRed(Sprintf("Couldn't send payload to logz.io. Error: %s", err)))
	}
}

func initializeLogzIO() (*logzioSender, error) {
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

	return (*logzioSender)(sender), nil
}

func StopAndDrainLogzio() {
	if logzioTransport != nil {
		(*logzio.LogzioSender)(logzioTransport).Stop()
	}
}
