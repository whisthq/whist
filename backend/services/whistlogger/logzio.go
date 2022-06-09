package whistlogger // import "github.com/whisthq/whist/backend/services/whistlogger"

import (
	"log"
	"time"

	"github.com/logzio/logzio-go"
	"github.com/whisthq/whist/backend/services/utils"
	"go.uber.org/zap/zapcore"
)

type logzioCore struct {
	enabler zapcore.LevelEnabler
	encoder zapcore.Encoder
	sender  *logzio.LogzioSender // logz client
}

func NewLogzioCore(encoder zapcore.Encoder, levelEnab zapcore.LevelEnabler) zapcore.Core {
	sender, err := logzio.New(
		"MoaZIzGkBxpsbbquDpwGlOTasLqKvtGJ",
		logzio.SetUrl("https://listener.logz.io:8071"),
		logzio.SetDrainDuration(time.Second*3),
		logzio.SetCheckDiskSpace(false),
	)
	if err != nil {
		log.Printf("Couldn't marshal payload for logz.io. Error: %s", err)
		return nil
	}

	lc := &logzioCore{}
	lc.encoder = encoder
	lc.enabler = levelEnab
	lc.sender = sender

	return lc
}

func NewLogzioEncoderConfig() zapcore.EncoderConfig {
	return zapcore.EncoderConfig{
		TimeKey:        "timestamp",
		LevelKey:       "type",
		NameKey:        "logger",
		CallerKey:      "caller",
		FunctionKey:    zapcore.OmitKey,
		MessageKey:     "message",
		StacktraceKey:  "stacktrace",
		LineEnding:     zapcore.DefaultLineEnding,
		EncodeLevel:    zapcore.LowercaseLevelEncoder,
		EncodeTime:     zapcore.EpochTimeEncoder,
		EncodeDuration: zapcore.SecondsDurationEncoder,
		EncodeCaller:   zapcore.ShortCallerEncoder,
	}
}

func (lc *logzioCore) Enabled(level zapcore.Level) bool {
	return lc.enabler.Enabled(level)
}

func (lc *logzioCore) With(fields []zapcore.Field) zapcore.Core {
	core := &logzioCore{
		enabler: lc.enabler,
		encoder: lc.encoder.Clone(),
		sender:  lc.sender,
	}

	for i := range fields {
		fields[i].AddTo(core.encoder)
	}

	return core
}

func (lc *logzioCore) Check(ent zapcore.Entry, ce *zapcore.CheckedEntry) *zapcore.CheckedEntry {
	if lc.Enabled(ent.Level) {
		return ce.AddCore(ent, lc)
	}
	return ce
}

func (lc *logzioCore) Write(ent zapcore.Entry, fields []zapcore.Field) error {
	buf, err := lc.encoder.EncodeEntry(ent, fields)
	if err != nil {
		return err
	}
	// Write to logzio
	err = lc.sender.Send(buf.Bytes())
	buf.Free()
	if err != nil {
		return utils.MakeError("Couldn't marshal payload for logz.io. Error: %s", err)
	}
	if ent.Level > zapcore.ErrorLevel {
		// Since we may be crashing the program, sync the output.
		lc.Sync()
	}
	return nil
}

func (lc *logzioCore) Sync() error {
	//Flush logzio
	return lc.sender.Sync()
}

// package whistlogger // import "github.com/whisthq/whist/backend/services/whistlogger"

// import (
// 	"encoding/json"
// 	"log"
// 	"os"
// 	"time"

// 	"github.com/whisthq/whist/backend/services/metadata"
// 	"github.com/whisthq/whist/backend/services/metadata/aws"
// 	"github.com/whisthq/whist/backend/services/utils"

// 	"github.com/logzio/logzio-go"
// )

// // We use a pointer of this type so we can check if it is nil in our logging
// // functions, and therefore always call them safely.
// var logzioTransport *logzioSender

// // We define a custom type to be able to override the Send() function with sane
// // error handling and our custom marshalling logic.
// type logzioSender logzio.LogzioSender

// type logzioMessageType string

// // message is a generic log message with basic data.
// type message struct {
// 	Message      string                  `json:"message"`
// 	Type         string                  `json:"type"`
// 	Component    string                  `json:"component"`
// 	SubComponent string                  `json:"sub_component"`
// 	Environment  metadata.AppEnvironment `json:"environment"`
// }

// // hostMessage includes data specific for the host service logs.
// type hostMessage struct {
// 	AWSInstanceID aws.InstanceID `json:"aws_instance_id"`
// 	AWSAmiID      aws.AmiID      `json:"aws_ami_id"`
// 	message
// }

// const (
// 	logzioTypeInfo    logzioMessageType = "info"
// 	logzioTypeWarning logzioMessageType = "warning"
// 	logzioTypeError   logzioMessageType = "error"
// )

// var hostMsg hostMessage
// var scalingMsg message

// func (sender *logzioSender) send(payload string, msgType logzioMessageType) {
// 	var (
// 		byteMsg []byte
// 		err     error
// 	)

// 	if hostMsg != (hostMessage{}) {
// 		hostMsg.Message = payload
// 		hostMsg.Type = string(msgType)
// 		byteMsg, err = json.Marshal(hostMsg)
// 	} else if scalingMsg != (message{}) {
// 		scalingMsg.Message = payload
// 		scalingMsg.Type = string(msgType)
// 		byteMsg, err = json.Marshal(scalingMsg)
// 	} else {
// 		return
// 	}

// 	if err != nil {
// 		log.Print(utils.ColorRed(utils.Sprintf("Couldn't marshal payload for logz.io. Error: %s", err)))
// 		return
// 	}

// 	err = (*logzio.LogzioSender)(sender).Send(byteMsg)
// 	if err != nil {
// 		log.Print(utils.ColorRed(utils.Sprintf("Couldn't send payload to logz.io. Error: %s", err)))
// 		return
// 	}
// }

// func initializeLogzIO() (*logzioSender, error) {
// 	if usingProdLogging() {
// 		Info("Setting up logz.io integration.")
// 	} else {
// 		Info("Not setting up logz.io integration.")
// 		return nil, nil
// 	}

// 	logzioShippingToken := os.Getenv("LOGZIO_SHIPPING_TOKEN")

// 	if logzioShippingToken == "" {
// 		return nil, utils.MakeError("Error initializing logz.io integration: logzioShippingToken is uninitialized")
// 	}

// 	sender, err := logzio.New(
// 		logzioShippingToken,
// 		logzio.SetUrl("https://listener.logz.io:8071"),
// 		logzio.SetDrainDuration(time.Second*3),
// 		logzio.SetCheckDiskSpace(false),
// 	)
// 	if err != nil {
// 		return nil, utils.MakeError("Error initializing logz.io integration: %s", err)
// 	}

// 	return (*logzioSender)(sender), nil
// }

// // FlushLogzio flushes events in the Logzio queue but does not stop new ones from being recorded.
// func FlushLogzio() {
// 	if logzioTransport != nil {
// 		if err := (*logzio.LogzioSender)(logzioTransport).Sync(); err != nil {
// 			Errorf("Unable to flush logzio: %s", err)
// 			return
// 		}

// 		(*logzio.LogzioSender)(logzioTransport).Drain()
// 	}
// }

// // stopAndDrainLogzio flushes events in the Logzio queue and stops new ones
// // from being recorded.
// func stopAndDrainLogzio() {
// 	if logzioTransport != nil {
// 		(*logzio.LogzioSender)(logzioTransport).Stop()
// 	}
// }
