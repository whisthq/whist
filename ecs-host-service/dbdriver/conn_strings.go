package dbdriver // import "github.com/fractal/fractal/ecs-host-service/dbdriver"

import (
	"github.com/fractal/fractal/ecs-host-service/metadata"
)

// Fractal database connection strings

const localDevDatabaseURL = "user=uap4ch2emueqo9 host=localhost port=9999 dbname=d9rf2k3vd6hvbm"

// The dev, staging, and prod ones are filled in by the linker
var devDatabaseURL string
var stagingDatabaseURL string
var prodDatabaseURL string

func getFractalDBConnString() string {
	switch metadata.GetAppEnvironment() {
	case metadata.EnvLocalDev:
		return "fake string to throw error if used, since EnvLocalDev should not connect to a database"
	case metadata.EnvLocalDevWithDB:
		return localDevDatabaseURL
	case metadata.EnvDev:
		return devDatabaseURL
	case metadata.EnvStaging:
		return stagingDatabaseURL
	case metadata.EnvProd:
		return prodDatabaseURL
	default:
		return localDevDatabaseURL
	}
}
