package metadata // import "github.com/fractal/fractal/host-service/metadata"

import (
	"os"
	"strings"
)

// An AppEnvironment represents either localdev or localdevwithdb (i.e. a dev
// instance), dev (i.e. talking to the dev webserver), staging, or prod
type AppEnvironment string

// Constants for the various AppEnvironments
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
