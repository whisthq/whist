package dbdriver // import "github.com/fractal/fractal/ecs-host-service/dbdriver"

import (
	"context"
	"sync"

	"github.com/jackc/pgx/v4/pgxpool"

	logger "github.com/fractal/fractal/ecs-host-service/fractallogger"
	"github.com/fractal/fractal/ecs-host-service/metadata"
	"github.com/fractal/fractal/ecs-host-service/utils"
)

// `enabled` is a flag denoting whether the functions in this package should do
// anything, or simply be no-ops. This is necessary, since we want the database
// operations to be meaningful in environments where we can expect the database
// guarantees to hold (i.e. `logger.EnvLocalDevWithDB` for now) but no-ops in
// other environments.
var enabled = (metadata.GetAppEnvironment() != metadata.EnvLocalDev)

// This is the connection pool for the database. It is an error for `dbpool` to
// be non-nil if `enabled` is true.
var dbpool *pgxpool.Pool

// Initialize creates and tests a connection to the database, and registers the
// instance in the database. It also starts the heartbeat goroutine.
func Initialize(globalCtx context.Context, globalCancel context.CancelFunc, goroutineTracker *sync.WaitGroup) error {
	if !enabled {
		logger.Infof("Running in app environment %s so not enabling database code.", metadata.GetAppEnvironment())
		return nil
	}

	// Ensure this function is not called multiple times
	if dbpool != nil {
		return utils.MakeError("dbdriver.Initialize() called multiple times!")
	}

	connStr := getFractalDBConnString()
	pgxConfig, err := pgxpool.ParseConfig(connStr)
	if err != nil {
		return utils.MakeError("Unable to parse database connection string! Error: %s", err)
	}

	// TODO: investigate and optimize the pgxConfig settings

	// We always want to have at least one connection open.
	pgxConfig.MinConns = 1

	// We want to test the connection right away and fail fast.
	pgxConfig.LazyConnect = false

	dbpool, err = pgxpool.ConnectConfig(globalCtx, pgxConfig)
	if err != nil {
		return utils.MakeError("Unable to connect to the database: %s", err)
	}
	logger.Infof("Successfully connected to the database.")

	// Register the instance with the database
	if err = registerInstance(globalCtx); err != nil {
		return utils.MakeError("Unable to register instance in the database: %s", err)
	}

	// Start goroutine that marks the host as draining if the global context is
	// cancelled.
	goroutineTracker.Add(1)
	go func() {
		defer goroutineTracker.Done()

		<-globalCtx.Done()
		logger.Infof("Global context cancelled, marking instance as draining in database...")
		if err := markDraining(context.Background()); err != nil {
			logger.Error(err)
		}
	}()

	// Initialize the heartbeat goroutine. Note that this goroutine does not
	// listen to the global context, and is not tracked by the global
	// goroutineTracker. This is because we want the heartbeat goroutine to
	// outlive the global context. Instead, we have the explicit Close(), which
	// ends up finishing off the heartbeat goroutine as well.
	go heartbeatGoroutine()

	return nil
}

// Close unregisters the instance and closes the connection pool to the
// database. We don't do this automatically in this package upon global context
// cancellation, since we'd only like to mark the host as draining at that
// time, but only close the database once we're actually about to shutdown. It
// also stops the heartbeat goroutine.
func Close() {
	logger.Infof("Closing the database driver...")

	// Stop the heartbeat goroutine
	close(heartbeatKeepAlive)

	// Delete the instance row
	if err := unregisterInstance(); err != nil {
		logger.Errorf("Error unregistering instance: %s", err)
	}

	logger.Infof("Closing the connection pool to the database...")
	if dbpool != nil {
		dbpool.Close()
	}
}
