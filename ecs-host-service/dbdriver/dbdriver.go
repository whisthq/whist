package dbdriver // import "github.com/fractal/fractal/ecs-host-service/dbdriver"

import (
	"context"
	"sync"

	"github.com/jackc/pgx/v4/pgxpool"

	logger "github.com/fractal/fractal/ecs-host-service/fractallogger"
)

// Fractal database connection strings
// TODO: figure out the rest of these
const localdevFractalDB = "user=uap4ch2emueqo9 host=localhost port=9999 dbname=d9rf2k3vd6hvbm"

func getFractalDBConnString() string {
	switch logger.GetAppEnvironment() {
	case logger.EnvLocalDev:
		return localdevFractalDB
	default:
		return localdevFractalDB
	}
}

var dbpool pgxpool.Pool

func Initialize(globalCtx context.Context, globalCancel context.CancelFunc, goroutineTracker *sync.WaitGroup) {
	dbpool, err := pgxpool.Connect(globalCtx, getFractalDBConnString())
	if err != nil {
		logger.Panicf(globalCancel, "Unable to connect to the database: %s", err)
		return
	}
	logger.Infof("Successfully connected to the database.")

	// Start goroutine that closes the connection pool if the global context is cancelled
	goroutineTracker.Add(1)
	go func() {
		defer goroutineTracker.Done()

		<-globalCtx.Done()
		logger.Infof("Closing the connection pool to the database...")
		if dbpool != nil {
			dbpool.Close()
		}
	}()
}

func main() {
	var greeting string
	err := dbpool.QueryRow(context.Background(), "select 'Hello, world!'").Scan(&greeting)
	if err != nil {
		logger.Errorf("QueryRow failed: %v\n", err)
	}

	logger.Info(greeting)
}
