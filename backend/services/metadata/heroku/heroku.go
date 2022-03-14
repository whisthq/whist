/*
Package heroku contains code to pull configuration variables from Heroku at runtime.
*/
package heroku // import "github.com/whisthq/whist/backend/services/metadata/heroku"

import (
	"github.com/bgentry/heroku-go"

	"github.com/whisthq/whist/backend/services/metadata"
)

// The following variables are filled in by the linker.
var email string
var apiKey string

// This one is optionally filled in by the linker. If set, it can be used to
// override the default logic used to determine which Heroku app to connect to.
var appNameOverride string

var client heroku.Client = heroku.Client{Username: email, Password: apiKey}

// GetAppName provides the Heroku app name to use based on the app environment
// the host service is running on, or the override if provided during build
// time. In a local environment, it defaults to the dev server.
func GetAppName() string {
	// Respect the override if set.
	if appNameOverride != "" {
		return appNameOverride
	}

	switch metadata.GetAppEnvironment() {
	case metadata.EnvDev:
		return "whist-dev-server"
	case metadata.EnvStaging:
		return "whist-staging-server"
	case metadata.EnvProd:
		return "whist-prod-server"
	default:
		// In the default case we use the dev server, like the webserver.
		return "whist-dev-server"
	}
}

// GetConfig returns the Heroku environment config for the app returned by
// GetAppName.
func GetConfig() (map[string]string, error) {
	return client.ConfigVarInfo(GetAppName())
}

// GetHasuraName provides the Heroku app name for the Hasura server to use
// based on the app environment the host service is running on, or the override
// if provided during build time. In a local environment,
// it defaults to the local hasura deployment.
func GetHasuraName() string {
	// Respect the override if set.
	if appNameOverride != "" {
		return appNameOverride
	}

	switch metadata.GetAppEnvironment() {
	case metadata.EnvDev:
		return "whist-dev-hasura"
	case metadata.EnvStaging:
		return "whist-staging-hasura"
	case metadata.EnvProd:
		return "whist-prod-hasura"
	default:
		// In the default case we use the dev hasura server.
		return "whist-dev-hasura"
	}
}

// GetConfigHasuraName provides the Heroku app name for the Hasura server that
// is connected to the configuration database.
func GetConfigHasuraName() string {
	return "whist-config-hasura"
}

// GetHasuraConfig returns the Heroku environment config for the hasura server returned by
// GetHasuraName.
func GetHasuraConfig() (map[string]string, error) {
	return client.ConfigVarInfo(GetHasuraName())
}

// GetConfigHasuraVars returns the Heroku environment config for the hasura server returned by
// GetConfigHasuraName.
func GetConfigHasuraVars() (map[string]string, error) {
	return client.ConfigVarInfo(GetConfigHasuraName())
}

// GetHasuraURL returns the Heroku web url for the hasura server returned by
// GetHasuraName.
func GetHasuraURL() (string, error) {
	app, err := client.AppInfo(GetHasuraName())
	if err != nil {
		return "", err
	}
	return app.WebURL + "v1/graphql", nil
}

// GetConfigHasuraURL returns the Heroku web url for the hasura server returned by
// GetHasuraName.
func GetConfigHasuraURL() (string, error) {
	app, err := client.AppInfo(GetConfigHasuraName())
	if err != nil {
		return "", err
	}
	return app.WebURL + "v1/graphql", nil
}
