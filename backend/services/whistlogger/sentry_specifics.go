package whistlogger // import "github.com/whisthq/whist/backend/services/whistlogger"

import (
	"log"
	"os"
	"time"

	"github.com/getsentry/sentry-go"
	"github.com/whisthq/whist/backend/services/metadata"

	"github.com/whisthq/whist/backend/services/utils"
)

type sentrySender struct{}

// We use a pointer of this type so we can check if it is nil in our logging
// functions, and therefore always call them safely.
var sentryTransport *sentrySender

func (*sentrySender) send(err error) {
	sentry.CaptureException(err)
}

// InitializeSentry initializes Sentry for use. It requires a configuration function `f`
// as an argument where the Sentry tags and messages should be set.
func initializeSentry(f func(scope *sentry.Scope)) (*sentrySender, error) {
	if usingProdLogging() {
		log.Print("Setting up Sentry.")
	} else {
		log.Print("Not setting up Sentry.")
		return nil, nil
	}

	sentryDsn := os.Getenv("SENTRY_DSN")

	err := sentry.Init(sentry.ClientOptions{
		Dsn:         sentryDsn,
		Release:     metadata.GetGitCommit(),
		Environment: string(metadata.GetAppEnvironment()),
	})
	if err != nil {
		return nil, utils.MakeError("Error calling Sentry.init: %v", err)
	}
	log.Printf("Set Sentry release to git commit hash: %s", metadata.GetGitCommit())

	// Configure Sentry's scope with some instance-specific information
	sentry.ConfigureScope(f)

	return new(sentrySender), nil
}

// FlushSentry flushes events in the Sentry queue
func FlushSentry() {
	sentry.Flush(5 * time.Second)
}
