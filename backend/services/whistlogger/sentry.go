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

type sentryCore struct {
	enabler zapcore.LevelEnabler
	encoder zapcore.Encoder
	sender  *sentry.Client
}

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

func (lc *sentryCore) Enabled(level zapcore.Level) bool {
	return lc.enabler.Enabled(level)
}

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

func (lc *sentryCore) Check(ent zapcore.Entry, ce *zapcore.CheckedEntry) *zapcore.CheckedEntry {
	if lc.Enabled(ent.Level) {
		return ce.AddCore(ent, lc)
	}
	return ce
}

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

func (lc *sentryCore) Sync() error {
	//Flush sentry
	ok := lc.sender.Flush(5 * time.Second)
	if !ok {
		return utils.MakeError("failed to flush Sentry, some events may not have been sent.")
	}

	return nil
}
