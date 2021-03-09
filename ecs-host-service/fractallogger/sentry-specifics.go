package fractallogger

import (
	"log"
	"time"

	"github.com/getsentry/sentry-go"
)

type sentrySender struct{}

// We use a pointer of this type so we can check if it is nil in our logging
// functions, and therefore always call them safely.
var sentryTransport *sentrySender

func (*sentrySender) send(err error) {
	sentry.CaptureException(err)
}

// InitializeSentry initializes Sentry for use.
func initializeSentry() (*sentrySender, error) {
	if usingProdLogging() {
		log.Print("Setting up Sentry.")
	} else {
		log.Print("Not setting up Sentry.")
		return nil, nil
	}

	err := sentry.Init(sentry.ClientOptions{
		Dsn:         "https://774bb2996acb4696944e1c847c41773c@o400459.ingest.sentry.io/5461239",
		Release:     GetGitCommit(),
		Environment: string(GetAppEnvironment()),
	})
	if err != nil {
		return nil, MakeError("Error calling Sentry.init: %v", err)
	}
	log.Printf("Set Sentry release to git commit hash: %s", GetGitCommit())

	// Configure Sentry's scope with some instance-specific information
	sentry.ConfigureScope(func(scope *sentry.Scope) {
		// This function looks repetitive, but we can't refactor its functionality
		// into a separate function because we want to defer sending the errors
		// about being unable to set Sentry tags until after we have set all the
		// ones we can.

		if val, err := GetAwsAmiID(); err != nil {
			defer Errorf("Unable to set Sentry tag aws.ami-id: %v", err)
		} else {
			scope.SetTag("aws.ami-id", val)
			log.Printf("Set Sentry tag aws.ami-id: %s", val)
		}

		if val, err := GetAwsAmiLaunchIndex(); err != nil {
			defer Errorf("Unable to set Sentry tag aws.ami-launch-index: %v", err)
		} else {
			scope.SetTag("aws.ami-launch-index", val)
			log.Printf("Set Sentry tag aws.ami-launch-index: %s", val)
		}

		if val, err := GetAwsInstanceID(); err != nil {
			defer Errorf("Unable to set Sentry tag aws.instance-id: %v", err)
		} else {
			scope.SetTag("aws.instance-id", val)
			log.Printf("Set Sentry tag aws.instance-id: %s", val)
		}

		if val, err := GetAwsInstanceType(); err != nil {
			defer Errorf("Unable to set Sentry tag aws.instance-type: %v", err)
		} else {
			scope.SetTag("aws.instance-type", val)
			log.Printf("Set Sentry tag aws.instance-type: %s", val)
		}

		if val, err := GetAwsPlacementRegion(); err != nil {
			defer Errorf("Unable to set Sentry tag aws.placement-region: %v", err)
		} else {
			scope.SetTag("aws.placement-region", val)
			log.Printf("Set Sentry tag aws.placement-region: %s", val)
		}

		if val, err := GetAwsPublicIpv4(); err != nil {
			defer Errorf("Unable to set Sentry tag aws.public-ipv4: %v", err)
		} else {
			scope.SetTag("aws.public-ipv4", val)
			log.Printf("Set Sentry tag aws.public-ipv4: %s", val)
		}
	})

	return new(sentrySender), nil
}

// FlushSentry flushes events in the Sentry queue
func FlushSentry() {
	sentry.Flush(5 * time.Second)
}
