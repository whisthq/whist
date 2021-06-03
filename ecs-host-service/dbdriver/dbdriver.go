package dbdriver // import "github.com/fractal/fractal/ecs-host-service/dbdriver"

import (
	"context"
	"os"
	"sync"

	"github.com/jackc/pgx/v4/pgxpool"

	logger "github.com/fractal/fractal/ecs-host-service/fractallogger"
)

var dbpool pgxpool.Pool

func Initialize(globalCtx context.Context, globalCancel context.CancelFunc, goroutineTracker *sync.WaitGroup) {
	url := os.Getenv("DATABASE_URL")
	dbpool, err := pgxpool.Connect(globalCtx, url)
	if err != nil {
		logger.Panicf(globalCancel, "Unable to connect to database with URL %v. Error: %s", url, err)
	}

	// Start goroutine that closes the connection pool if the global context is cancelled
	goroutineTracker.Add(1)
	go func() {
		defer goroutineTracker.Done()

		<-globalCtx.Done()
		logger.Infof("Closing the connection pool to the database...")
		dbpool.Close()
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
