package logger

import (
	"fmt"
	"log"
	"os"
	"time"

	"github.com/logzio/logzio-go"
	"go.uber.org/zap"
	"go.uber.org/zap/zapcore"
)

// logzioCore is a custom core that sends output to Logz.io
type logzioCore struct {
	// enabler decides whether the entry should be logged or not,
	// according to its level.
	enabler zapcore.LevelEnabler
	// encoder is responsible for marshalling the entry to the desired format.
	encoder zapcore.Encoder
	// sender is the client used to send the events to Logz.io
	sender *logzio.LogzioSender
}

// fields are additional fields to be parsed by Logz.io
var messageFields []zapcore.Field

// NewLogzioCore will initialize logz and necessary fields.
func newLogzioCore(encoder zapcore.Encoder, levelEnab zapcore.LevelEnabler) zapcore.Core {
	logzioShippingToken := os.Getenv("LOGZIO_SHIPPING_TOKEN")
	if logzioShippingToken == "" {
		// Here we use log because the whistlogger hasn't been
		// fully initialized.
		log.Printf("Logz shipping token is empty! Returning a no-op logging core.")
		return zapcore.NewNopCore()
	}

	sender, err := logzio.New(
		logzioShippingToken,
		logzio.SetUrl("https://listener.logz.io:8071"),
		logzio.SetDrainDuration(time.Second*3),
		logzio.SetCheckDiskSpace(false),
	)
	if err != nil {
		log.Printf("couldn't marshal payload for logz.io: %s", err)
		return nil
	}

	lc := &logzioCore{}
	lc.encoder = encoder
	lc.enabler = levelEnab
	lc.sender = sender

	return lc
}

// NewLogzioEncoderConfig returns a configuration that is appropiate for
// using with logz.io.
func newLogzioEncoderConfig() zapcore.EncoderConfig {
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

// AddLogzFields will add the fields to the Logzio message.
func AddLogzioFields(fields map[string]string) {
	for name, val := range fields {
		messageFields = append(messageFields, zap.String(name, val))
	}
}

// Enabled is used to check whether the event should be logged
// or not, depending on its level.
func (lc *logzioCore) Enabled(level zapcore.Level) bool {
	return lc.enabler.Enabled(level)
}

// With adds the fields defined in the configuration to the core.
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

// Check will add the current entry (event) to the core, which in the future will
// send it to logz.io.
func (lc *logzioCore) Check(ent zapcore.Entry, ce *zapcore.CheckedEntry) *zapcore.CheckedEntry {
	if lc.Enabled(ent.Level) {
		return ce.AddCore(ent, lc)
	}
	return ce
}

// Write is where the core sends the event payload to logz.io
func (lc *logzioCore) Write(ent zapcore.Entry, fields []zapcore.Field) error {
	// TODO
	if true {
		return nil
	}

	fields = append(fields, messageFields...)
	buf, err := lc.encoder.EncodeEntry(ent, fields)
	if err != nil {
		return err
	}
	// Write to logzio
	err = lc.sender.Send(buf.Bytes())
	buf.Free()
	if err != nil {
		return fmt.Errorf("couldn't marshal payload for logz.io: %s", err)
	}
	if ent.Level > zapcore.ErrorLevel {
		// Since we may be crashing the program, sync the output.
		lc.Sync()
	}

	return nil
}

// Sync drains the queue.
func (lc *logzioCore) Sync() error {
	//Flush logzio
	messageFields = nil
	return lc.sender.Sync()
}
