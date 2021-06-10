package dbdriver // import "github.com/fractal/fractal/ecs-host-service/dbdriver"

import (
	logger "github.com/fractal/fractal/ecs-host-service/fractallogger"
)

// Fractal database connection strings
// TODO: figure out the rest of these
const localdevFractalDB = "user=uap4ch2emueqo9 host=localhost port=9999 dbname=d9rf2k3vd6hvbm"

func getFractalDBConnString() string {
	switch logger.GetAppEnvironment() {
	case logger.EnvLocalDev:
		return "fake string to throw error if used, since EnvLocalDev should not connect to a database"
	case logger.EnvLocalDevWithDB:
		return localdevFractalDB
	default:
		return localdevFractalDB
	}
}
