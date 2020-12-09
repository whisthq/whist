package fractallogger

import (
	"log"
	"os"
	"strings"
	"time"

	"github.com/getsentry/sentry-go"
)

func InitializeSentry() error {
	// Only set up Sentry to log events if running in production
	strProd := os.Getenv("USE_PROD_SENTRY")
	useProdSentry := (strProd == "1") || (strings.ToLower(strProd) == "yes") || (strings.ToLower(strProd) == "true") || (IsRunningInProduction())
	if useProdSentry {
		log.Print("Using production sentry configuration.")
	} else {
		log.Print("Not production - not setting up sentry")
		return nil
	}

	err := sentry.Init(sentry.ClientOptions{
		Dsn:     "https://774bb2996acb4696944e1c847c41773c@o400459.ingest.sentry.io/5461239",
		Release: GetGitCommit(),
		Environment: os.Getenv("APP_ENV")
	})
	if err != nil {
		return MakeError("Error calling Sentry.init: %v", err)
	}
	log.Printf("Set sentry release to git commit hash: %s", GetGitCommit())

	// Configure Sentry's scope with some instance-specific information
	sentry.ConfigureScope(func(scope *sentry.Scope) {
		// This function looks repetitive, but we can't refactor its functionality
		// into a separate function because we want to defer sending the errors
		// about being unable to set Sentry tags until after we have set all the
		// ones we can.

		if val, err := GetAwsAmiId(); err != nil {
			defer Errorf("Unable to set Sentry tag aws.ami-id: %v", err)
		} else {
			scope.SetTag("aws.ami-id", val)
			log.Printf("Set sentry tag aws.ami-id: %s", val)
		}

		if val, err := GetAwsAmiLaunchIndex(); err != nil {
			defer Errorf("Unable to set Sentry tag aws.ami-launch-index: %v", err)
		} else {
			scope.SetTag("aws.ami-launch-index", val)
			log.Printf("Set sentry tag aws.ami-launch-index: %s", val)
		}

		if val, err := GetAwsInstanceId(); err != nil {
			defer Errorf("Unable to set Sentry tag aws.instance-id: %v", err)
		} else {
			scope.SetTag("aws.instance-id", val)
			log.Printf("Set sentry tag aws.instance-id: %s", val)
		}

		if val, err := GetAwsInstanceType(); err != nil {
			defer Errorf("Unable to set Sentry tag aws.instance-type: %v", err)
		} else {
			scope.SetTag("aws.instance-type", val)
			log.Printf("Set sentry tag aws.instance-type: %s", val)
		}

		if val, err := GetAwsPlacementRegion(); err != nil {
			defer Errorf("Unable to set Sentry tag aws.placement-region: %v", err)
		} else {
			scope.SetTag("aws.placement-region", val)
			log.Printf("Set sentry tag aws.placement-region: %s", val)
		}

		if val, err := GetAwsPublicIpv4(); err != nil {
			defer Errorf("Unable to set Sentry tag aws.public-ipv4: %v", err)
		} else {
			scope.SetTag("aws.public-ipv4", val)
			log.Printf("Set sentry tag aws.public-ipv4: %s", val)
		}
	})
	return nil
}

func FlushSentry() {
	sentry.Flush(5 * time.Second)
}
