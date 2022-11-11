package logger

import (
	"errors"
	"fmt"
	"log"
	"os"
	"reflect"
	"time"

	"github.com/getsentry/sentry-go"
	"github.com/whisthq/whist/backend/elegant_monolith/internal/config"
	"go.uber.org/zap/zapcore"
)

// sentryCore is a custom core that sends output to Sentry
type sentryCore struct {
	// enabler decides whether the entry should be logged or not,
	// according to its level.
	enabler zapcore.LevelEnabler
	// encoder is responsible for marshalling the entry to the desired format.
	encoder zapcore.Encoder
	// sender is the client used to send the events to Sentry.
	sender *sentry.Client
}

// NewSentryCore will initialize sentry and necessary fields.
func newSentryCore(encoder zapcore.Encoder, levelEnab zapcore.LevelEnabler) zapcore.Core {
	sentryDsn := os.Getenv("SENTRY_DSN")
	if sentryDsn == "" {
		// Here we use log because the whistlogger hasn't been
		// fully
		log.Printf("Sentry DSN is empty! Returning a no-op logging core.")
		return zapcore.NewNopCore()
	}

	sender, err := sentry.NewClient(sentry.ClientOptions{
		Dsn: sentryDsn,
		//Release:     metadata.GetGitCommit(),
		// Environment: string(metadata.GetAppEnvironment()),
	})
	if err != nil {
		log.Printf("error starting Sentry client: %s", err)
		return nil
	}
	// log.Printf("Set Sentry release to git commit hash: %s", metadata.GetGitCommit())

	sc := &sentryCore{}
	sc.encoder = encoder
	sc.enabler = levelEnab
	sc.sender = sender

	return sc
}

// NewSentryEncoderConfig returns a configuration that is appropiate for
// using with sentry.
func newSentryEncoderConfig() zapcore.EncoderConfig {
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

// AddSentryTags will add the tags to the current Sentry scope
func AddSentryTags(tags map[string]string) {
	sentry.ConfigureScope(func(scope *sentry.Scope) {
		scope.SetTags(tags)
	})
}

// Enabled is used to check whether the event should be logged
// or not, depending on its level.
func (sc *sentryCore) Enabled(level zapcore.Level) bool {
	return sc.enabler.Enabled(level)
}

// With adds the fields defined in the configuration to the core.
func (sc *sentryCore) With(fields []zapcore.Field) zapcore.Core {
	core := &logzioCore{
		enabler: sc.enabler,
		encoder: sc.encoder.Clone(),
	}

	for i := range fields {
		fields[i].AddTo(core.encoder)
	}

	return core
}

// Check will add the current entry (event) to the core, which in the future will
// send it to Sentry.
func (sc *sentryCore) Check(ent zapcore.Entry, ce *zapcore.CheckedEntry) *zapcore.CheckedEntry {
	if sc.Enabled(ent.Level) {
		return ce.AddCore(ent, sc)
	}
	return ce
}

// Write is where the core sends the event payload to Sentry. This method
// will manually assemble Sentry events so that they are sent correctly.
func (sc *sentryCore) Write(ent zapcore.Entry, fields []zapcore.Field) error {
	if !config.UseProdLogging() {
		return nil
	}

	// Assemble Sentry event
	err := errors.New(ent.Message)
	event := sentry.NewEvent()
	event.Level = sentry.Level(ent.Level.String())
	event.Exception = append(event.Exception, sentry.Exception{
		Value:      ent.Message,
		Type:       reflect.TypeOf(err).String(),
		Stacktrace: getStackStrace(err),
	})
	event.Timestamp = ent.Time

	// Add fields to a derived Sentry scope. This will not
	// modify the original scope.
	sentry.WithScope(func(scope *sentry.Scope) {
		for _, field := range fields {
			if field.String != "" {
				scope.SetTag(field.Key, field.String)
			}
		}

		// Send to Sentry
		sc.sender.CaptureEvent(event, &sentry.EventHint{OriginalException: err}, scope)
	})

	return nil
}

// Sync will send all events to Sentry and flush the queue.
func (sc *sentryCore) Sync() error {
	//Flush sentry
	ok := sc.sender.Flush(5 * time.Second)
	if !ok {
		return fmt.Errorf("failed to flush Sentry, some events may not have been sent")
	}

	return nil
}

// getStackTrace will extract and filter the stack trace from the error.
func getStackStrace(err error) *sentry.Stacktrace {
	stack := sentry.ExtractStacktrace(err)
	stacktrace := sentry.Stacktrace{
		Frames: stack.Frames,
	}

	return &stacktrace
}
