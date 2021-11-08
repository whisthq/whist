package metadata // import "github.com/fractal/fractal/host-service/metadata"

import (
	"os"
	"strings"

	mandelboxtypes "github.com/fractal/fractal/host-service/mandelbox/types"
	"github.com/fractal/fractal/host-service/metadata/aws"
	"github.com/fractal/fractal/host-service/utils"
)

func init() {
	// Verify that the host service is running on a valid environment. Panic if not.
	if err := verifyEnvironment(); err != nil {
		panic(err)
	}
}

// An AppEnvironment represents either localdev or localdevwithdb (i.e. a personal
// instance), dev (i.e. talking to the dev webserver), staging, or prod
type AppEnvironment string

// Constants for the various AppEnvironments. DO NOT CHANGE THESE without
// understanding how any consumers of GetAppEnvironment() and
// GetAppEnvironmentLowercase() are using them!
const (
	EnvLocalDevWithDB AppEnvironment = "LOCALDEVWITHDB"
	EnvLocalDev       AppEnvironment = "LOCALDEV"
	EnvDev            AppEnvironment = "DEV"
	EnvStaging        AppEnvironment = "STAGING"
	EnvProd           AppEnvironment = "PROD"
)

// GetAppEnvironment returns the AppEnvironment of the current instance.
var GetAppEnvironment func() AppEnvironment = func(unmemoized func() AppEnvironment) func() AppEnvironment {
	// This nested function syntax is used to memoize the result of the first call
	// to GetAppEnvironment() and cache the result for all future calls.

	var isCached = false
	var cache AppEnvironment

	return func() AppEnvironment {
		if isCached {
			return cache
		}
		cache = unmemoized()
		isCached = true
		return cache
	}
}(func() AppEnvironment {
	// Caching-agnostic logic goes here
	env := strings.ToLower(os.Getenv("APP_ENV"))
	switch env {
	case "development", "dev":
		return EnvDev
	case "staging":
		return EnvStaging
	case "production", "prod":
		return EnvProd
	case "localdevwithdb", "localdev_with_db", "localdev_with_database":
		return EnvLocalDevWithDB
	default:
		return EnvLocalDev
	}
})

// IsLocalEnv returns true if this host service is running locally for
// development.
func IsLocalEnv() bool {
	env := GetAppEnvironment()
	return env == EnvLocalDev || env == EnvLocalDevWithDB
}

// GetAppEnvironmentLowercase returns the app environment string, but just
// converted to lowercase. This is helpful to construct larger strings (i.e.
// Docker image names) that depend on the current environment.
func GetAppEnvironmentLowercase() string {
	return strings.ToLower(string(GetAppEnvironment()))
}

// IsRunningInCI returns true if the host service is running in continuous
// integration (i.e. for tests), and false otherwise.
func IsRunningInCI() bool {
	strCI := strings.ToLower(os.Getenv("CI"))
	switch strCI {
	case "1", "yes", "true", "on", "yep":
		return true
	case "0", "no", "false", "off", "nope":
		return false
	default:
		return false
	}
}

// GetUserID returns the user ID depending on the environment
// the host is run.
func GetUserID() (mandelboxtypes.UserID, error) {
	var UserID mandelboxtypes.UserID
	if IsRunningInCI() {
		// CI doesn't run in AWS so we need to set a custom name
		UserID = "localdev_host_service_CI"
	} else {
		instanceName, err := aws.GetInstanceName()
		if err != nil {
			return "", utils.MakeError("Error getting instance name from AWS, %v", err)
		}
		UserID = mandelboxtypes.UserID(utils.Sprintf("localdev_host_service_user_%s", instanceName))
	}

	return UserID, nil
}

// verifyEnvironment is used to enforce that the host service is running on a valid
// environment.
func verifyEnvironment() error {
	if IsRunningInCI() && !IsLocalEnv() {
		// Running on a non-local environment with CI enabled is an invalid configuration.
		return utils.MakeError("Running on non-local environment with CI enabled.")
	}

	return nil
}
