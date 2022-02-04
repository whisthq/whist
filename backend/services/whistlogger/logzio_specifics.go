package whistlogger // import "github.com/whisthq/whist/backend/services/whistlogger"

import (
	"encoding/json"
	"log"
	"os"
	"time"

	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/metadata/aws"
	"github.com/whisthq/whist/backend/services/utils"

	"github.com/logzio/logzio-go"
)

// We use a pointer of this type so we can check if it is nil in our logging
// functions, and therefore always call them safely.
var logzioTransport *logzioSender

// We define a custom type to be able to override the Send() function with sane
// error handling and our custom marshalling logic.
type logzioSender logzio.LogzioSender

type logzioMessageType string

// message is a generic log message with basic data.
type message struct {
	Message      string                  `json:"message"`
	Type         string                  `json:"type"`
	Component    string                  `json:"component"`
	SubComponent string                  `json:"sub_component"`
	Environment  metadata.AppEnvironment `json:"environment"`
}

// hostMessage includes data specific for the host service logs.
type hostMessage struct {
	AWSInstanceID aws.InstanceID `json:"aws_instance_id"`
	AWSAmiID      aws.AmiID      `json:"aws_ami_id"`
	message
}

const (
	logzioTypeInfo    logzioMessageType = "info"
	logzioTypeWarning logzioMessageType = "warning"
	logzioTypeError   logzioMessageType = "error"
)

var hostMsg hostMessage
var scalingMsg message

func (sender *logzioSender) send(payload string, msgType logzioMessageType) {
	var (
		byteMsg []byte
		err     error
	)

	if hostMsg != (hostMessage{}) {
		hostMsg.Message = payload
		hostMsg.Type = string(msgType)
		byteMsg, err = json.Marshal(hostMsg)
	} else if scalingMsg != (message{}) {
		scalingMsg.Message = payload
		scalingMsg.Type = string(msgType)
		byteMsg, err = json.Marshal(scalingMsg)
	} else {
		return
	}

	if err != nil {
		log.Print(utils.ColorRed(utils.Sprintf("Couldn't marshal payload for logz.io. Error: %s", err)))
		return
	}

	err = (*logzio.LogzioSender)(sender).Send(byteMsg)
	if err != nil {
		log.Print(utils.ColorRed(utils.Sprintf("Couldn't send payload to logz.io. Error: %s", err)))
		return
	}
}

func initializeLogzIO() (*logzioSender, error) {
	if usingProdLogging() {
		Info("Setting up logz.io integration.")
	} else {
		Info("Not setting up logz.io integration.")
		return nil, nil
	}

	logzioShippingToken := os.Getenv("LOGZIO_SHIPPING_TOKEN")

	if logzioShippingToken == "" {
		return nil, utils.MakeError("Error initializing logz.io integration: logzioShippingToken is uninitialized")
	}

	sender, err := logzio.New(
		logzioShippingToken,
		logzio.SetUrl("https://listener.logz.io:8071"),
		logzio.SetDrainDuration(time.Second*3),
		logzio.SetCheckDiskSpace(false),
	)
	if err != nil {
		return nil, utils.MakeError("Error initializing logz.io integration: %s", err)
	}

	return (*logzioSender)(sender), nil
}

// FlushLogzio flushes events in the Logzio queue but does not stop new ones from being recorded.
func FlushLogzio() {
	if logzioTransport != nil {
		if err := (*logzio.LogzioSender)(logzioTransport).Sync(); err != nil {
			Errorf("Unable to flush logzio: %s", err)
			return
		}

		(*logzio.LogzioSender)(logzioTransport).Drain()
	}
}

// stopAndDrainLogzio flushes events in the Logzio queue and stops new ones
// from being recorded.
func stopAndDrainLogzio() {
	if logzioTransport != nil {
		(*logzio.LogzioSender)(logzioTransport).Stop()
	}
}
