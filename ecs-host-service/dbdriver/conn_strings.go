package dbdriver // import "github.com/fractal/fractal/ecs-host-service/dbdriver"

import (
	"github.com/fractal/fractal/ecs-host-service/metadata"
)

// Fractal database connection strings
// TODO: figure out the rest of these
const localdevFractalDB = "user=uap4ch2emueqo9 host=localhost port=9999 dbname=d9rf2k3vd6hvbm"

func getFractalDBConnString() string {
	switch metadata.GetAppEnvironment() {
	case metadata.EnvLocalDev:
		return "fake string to throw error if used, since EnvLocalDev should not connect to a database"
	case metadata.EnvLocalDevWithDB:
		return localdevFractalDB
	default:
		return localdevFractalDB
	}
}
