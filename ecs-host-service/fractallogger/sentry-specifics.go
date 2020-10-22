package fractallogger

import (
	"log"
	"os"
	"strings"
	"time"

	"github.com/getsentry/sentry-go"
)

func InitializeSentry() error {
	strProd := os.Getenv("USE_PROD_SENTRY")
	// We want to use the production sentry config if we run with that
	// environment variable, or if we are actually running in production.
	useProdSentry := (strProd == "1") || (strings.ToLower(strProd) == "yes") || (strings.ToLower(strProd) == "true") || (IsRunningInProduction())
	if useProdSentry {
		log.Print("Using production sentry configuration: Debug: false")
	} else {
		log.Print("Using debug sentry configuration: Debug: true. Printing sentry events instead")
	}

	err := sentry.Init(sentry.ClientOptions{
		Dsn:     "https://5f2dd9674e2b4546b52c205d7382ac90@o400459.ingest.sentry.io/5461239",
		Release: "0.1",
		Debug:   !useProdSentry,
	})
	if err != nil {
		return MakeError("Error calling Sentry.init: %v", err)
	}

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
