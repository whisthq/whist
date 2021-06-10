package fractallogger // import "github.com/fractal/fractal/ecs-host-service/fractallogger"

import (
	"os"
	"strings"
)

// An EnvironmentType represents either localdev or localdevwithdb (i.e. a dev
// instance), dev (i.e. talking to the dev websever), staging, or prod
type EnvironmentType string

// Constants for the various EnvironmentTypes
const (
	EnvLocalDevWithDB EnvironmentType = "LOCALDEVWITHDB"
	EnvLocalDev       EnvironmentType = "LOCALDEV"
	EnvDev            EnvironmentType = "DEV"
	EnvStaging        EnvironmentType = "STAGING"
	EnvProd           EnvironmentType = "PROD"
)

// GetAppEnvironment returns the EnvironmentType of the current instance.
var GetAppEnvironment func() EnvironmentType = func(unmemoized func() EnvironmentType) func() EnvironmentType {
	// This nested function syntax is used to memoize the result of the first call
	// to GetAppEnvironment() and cache the result for all future calls.

	var isCached = false
	var cache EnvironmentType

	return func() EnvironmentType {
		if isCached {
			return cache
		}
		cache = unmemoized()
		isCached = true
		return cache
	}
}(func() EnvironmentType {
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
