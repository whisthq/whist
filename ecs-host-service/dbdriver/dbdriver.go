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

var dbpool *pgxpool.Pool

func Initialize(globalCtx context.Context, globalCancel context.CancelFunc, goroutineTracker *sync.WaitGroup) {
	connStr := getFractalDBConnString()
	pgxConfig, err := pgxpool.ParseConfig(connStr)
	if err != nil {
		logger.Panicf(globalCancel, "Unable to parse database connection string! Error: %s", err)
		return
	}
	// TODO: investigate and optimize the pgxConfig settings
	// We always want to have at least one connection open.
	pgxConfig.MinConns = 1
	pgxConfig.LazyConnect = false

	dbpool, err = pgxpool.ConnectConfig(globalCtx, pgxConfig)
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
