package whistlogger // import "github.com/whisthq/whist/backend/services/whistlogger"

import (
	"log"
	"reflect"
	"time"

	"github.com/getsentry/sentry-go"
	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/utils"
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

//  NewSentryCore will initialize sentry and necessary fields.
func NewSentryCore(encoder zapcore.Encoder, levelEnab zapcore.LevelEnabler) zapcore.Core {
	// sentryDsn := os.Getenv("SENTRY_DSN")
	sentryDsn := "https://52baabcbf5cb4cd9a83c825ffd55cb4c@o400459.ingest.sentry.io/6144258"
	sender, err := sentry.NewClient(sentry.ClientOptions{
		Dsn:         sentryDsn,
		Release:     metadata.GetGitCommit(),
		Environment: string(metadata.GetAppEnvironment()),
	})
	if err != nil {
		log.Printf("Error starting Sentry client: %s", err)
		return nil
	}
	log.Printf("Set Sentry release to git commit hash: %s", metadata.GetGitCommit())

	lc := &sentryCore{}
	lc.encoder = encoder
	lc.enabler = levelEnab
	lc.sender = sender

	return lc
}

// NewSentryEncoderConfig returns a configuration that is appropiate for
// using with sentry.
func NewSentryEncoderConfig() zapcore.EncoderConfig {
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

// Enabled is used to check whether the event should be logged
// or not, depending on its level.
func (lc *sentryCore) Enabled(level zapcore.Level) bool {
	return lc.enabler.Enabled(level)
}

// With adds the fields defined in the configuration to the core.
func (lc *sentryCore) With(fields []zapcore.Field) zapcore.Core {
	core := &logzioCore{
		enabler: lc.enabler,
		encoder: lc.encoder.Clone(),
	}

	for i := range fields {
		fields[i].AddTo(core.encoder)
	}

	return core
}

// Check will add the current entry (event) to the core, which in the future will
// send it to Sentry.
func (lc *sentryCore) Check(ent zapcore.Entry, ce *zapcore.CheckedEntry) *zapcore.CheckedEntry {
	if lc.Enabled(ent.Level) {
		return ce.AddCore(ent, lc)
	}
	return ce
}

// Write is where the core sends the event payload to Sentry. This method
// will manually assemble Sentry events so that they are sent correctly.
func (lc *sentryCore) Write(ent zapcore.Entry, fields []zapcore.Field) error {
	if usingProdLogging() {
		return nil
	}

	// Write to sentry
	err := utils.MakeError(ent.Message)
	event := sentry.NewEvent()
	event.Level = sentry.Level(ent.Level.String())
	event.Exception = append(event.Exception, sentry.Exception{
		Value:      ent.Message,
		Type:       reflect.TypeOf(err).String(),
		Stacktrace: sentry.ExtractStacktrace(err),
	})
	event.Timestamp = ent.Time

	lc.sender.CaptureEvent(event, &sentry.EventHint{OriginalException: utils.MakeError(ent.Message)}, sentry.CurrentHub().Scope())
	return nil
}

// Sync will send all events to Sentry and flush the queue.
func (lc *sentryCore) Sync() error {
	//Flush sentry
	ok := lc.sender.Flush(5 * time.Second)
	if !ok {
		return utils.MakeError("failed to flush Sentry, some events may not have been sent.")
	}

	return nil
}
