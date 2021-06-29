package heroku // import "github.com/fractal/fractal/ecs-host-service/metadata/heroku"

import (
	"github.com/bgentry/heroku-go"

	"github.com/fractal/fractal/ecs-host-service/metadata"
)

// This file contains code to pull configuration variables from Heroku at runtime.

// The following variables are filled in by the linker.
var email string
var apiKey string

var client heroku.Client = heroku.Client{Username: email, Password: apiKey}

func GetAppName() string {
	switch metadata.GetAppEnvironment() {
	case metadata.EnvDev:
		return "fractal-dev-server"
	case metadata.EnvStaging:
		return "fractal-staging-server"
	case metadata.EnvProd:
		return "fractal-prod-server"
	default:
		// In the default case we use the dev server, like the webserver.
		return "fractal-dev-server"
	}
}

func GetConfig() (map[string]string, error) {
	return client.ConfigVarInfo(GetAppName())
}
