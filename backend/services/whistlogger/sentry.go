package whistlogger // import "github.com/whisthq/whist/backend/services/whistlogger"

import (
	"log"
	"time"

	"github.com/getsentry/sentry-go"
	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/utils"
	"go.uber.org/zap/zapcore"
)

type sentryCore struct {
	enabler zapcore.LevelEnabler
	encoder zapcore.Encoder
}

func NewSentryCore(encoder zapcore.Encoder, levelEnab zapcore.LevelEnabler) zapcore.Core {
	// sentryDsn := os.Getenv("SENTRY_DSN")
	sentryDSN := "https://774bb2996acb4696944e1c847c41773c@o400459.ingest.sentry.io/5461239"
	err := sentry.Init(sentry.ClientOptions{
		Dsn:         sentryDSN,
		Release:     metadata.GetGitCommit(),
		Environment: string(metadata.GetAppEnvironment()),
	})
	if err != nil {
		log.Printf("Error calling Sentry.init: %v", err)
		return nil
	}
	log.Printf("Set Sentry release to git commit hash: %s", metadata.GetGitCommit())

	lc := &sentryCore{}
	lc.encoder = encoder
	lc.enabler = levelEnab

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
	// Write to sentry

	sentry.CaptureException(utils.MakeError(ent.Message))

	if ent.Level > zapcore.ErrorLevel {
		// Since we may be crashing the program, sync the output.
		lc.Sync()
	}
	return nil
}

func (lc *sentryCore) Sync() error {
	//Flush sentry
	ok := sentry.Flush(5 * time.Second)
	if !ok {
		return utils.MakeError("failed to flush Sentry, some events may not have been sent.")
	}

	return nil
}

// import (
// 	"log"
// 	"os"
// 	"time"

// 	"github.com/getsentry/sentry-go"
// 	"github.com/whisthq/whist/backend/services/metadata"

// 	"github.com/whisthq/whist/backend/services/utils"
// )

// type sentrySender struct{}

// // We use a pointer of this type so we can check if it is nil in our logging
// // functions, and therefore always call them safely.
// var sentryTransport *sentrySender

// func (*sentrySender) send(err error) {
// 	sentry.CaptureException(err)
// }

// // InitializeSentry initializes Sentry for use. It requires a configuration function `f`
// // as an argument where the Sentry tags and messages should be set.
// func initializeSentry(f func(scope *sentry.Scope)) (*sentrySender, error) {
// 	if usingProdLogging() {
// 		log.Print("Setting up Sentry.")
// 	} else {
// 		log.Print("Not setting up Sentry.")
// 		return nil, nil
// 	}

// 	sentryDsn := os.Getenv("SENTRY_DSN")

// 	if sentryDsn == "" {
// 		return nil, utils.MakeError("Error initializing Sentry integration: Sentry Dsn is uninitialized")
// 	}

// 	err := sentry.Init(sentry.ClientOptions{
// 		Dsn:         sentryDsn,
// 		Release:     metadata.GetGitCommit(),
// 		Environment: string(metadata.GetAppEnvironment()),
// 	})
// 	if err != nil {
// 		return nil, utils.MakeError("Error calling Sentry.init: %v", err)
// 	}
// 	log.Printf("Set Sentry release to git commit hash: %s", metadata.GetGitCommit())

// 	// Configure Sentry's scope with some instance-specific information
// 	sentry.ConfigureScope(f)

// 	return new(sentrySender), nil
// }

// // FlushSentry flushes events in the Sentry queue
// func FlushSentry() {
// 	sentry.Flush(5 * time.Second)
// }
