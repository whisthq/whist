package dbdriver // import "github.com/fractal/fractal/ecs-host-service/dbdriver"

import (
	"context"
	"sync"
	"time"

	"github.com/jackc/pgx/v4/pgxpool"

	logger "github.com/fractal/fractal/ecs-host-service/fractallogger"
	"github.com/fractal/fractal/ecs-host-service/metadata"
	"github.com/fractal/fractal/ecs-host-service/metadata/aws"
	"github.com/fractal/fractal/ecs-host-service/metrics"
	"github.com/fractal/fractal/ecs-host-service/utils"

	"github.com/fractal/fractal/ecs-host-service/dbdriver/queries"
)

// `enabled` is a flag denoting whether the functions in this package should do
// anything, or simply be no-ops. This is necessary, since we want the database
// operations to be meaningful in environments where we can expect the database
// guarantees to hold (i.e. `logger.EnvLocalDevWithDB` for now) but no-ops in
// other environments.
var enabled = (metadata.GetAppEnvironment() == metadata.EnvLocalDevWithDB)

// This is the connection pool for the database. It is an error for `dbpool` to
// be non-nil if `enabled` is true.
var dbpool *pgxpool.Pool

// Initialize creates and tests a connection to the database, and registers the
// instance in the database.
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

	// Start goroutine that closes the connection pool if the global context is
	// cancelled. Note that we do this before calling `registerInstance` so that
	// even if it fails, we successfully remove our row from the database when
	// cleaning up.
	goroutineTracker.Add(1)
	go func() {
		defer goroutineTracker.Done()

		<-globalCtx.Done()
		logger.Infof("Global context cancelled, removing instance from database...")
		if err := unregisterInstance(); err != nil {
			logger.Errorf("Error unregistering instance: %s", err)
		}

		logger.Infof("Closing the connection pool to the database...")
		if dbpool != nil {
			dbpool.Close()
		}
	}()

	// Register the instance with the database
	if err = registerInstance(globalCtx); err != nil {
		return utils.MakeError("Unable to register instance in the database: %s", err)
	}

	return nil
}

// registerInstance looks for and "takes over" the relevant row in the
// database. If the expected row is not found, then it returns an error.
func registerInstance(ctx context.Context) error {
	if !enabled {
		return nil
	}
	if dbpool == nil {
		return utils.MakeError("registerInstance() called but dbdriver is not initialized!")
	}

	instanceName, err := aws.GetInstanceName(ctx)
	if err != nil {
		return utils.MakeError("Couldn't register instance: couldn't get instance name: %s", err)
	}
	publicIP4, err := aws.GetPublicIpv4()
	if err != nil {
		return utils.MakeError("Couldn't register instance: couldn't get public IPv4: %s", err)
	}
	amiID, err := aws.GetAmiID()
	if err != nil {
		return utils.MakeError("Couldn't register instance: couldn't get AMI ID: %s", err)
	}
	region, err := aws.GetPlacementRegion()
	if err != nil {
		return utils.MakeError("Couldn't register instance: couldn't get AWS Placement Region: %s", err)
	}
	instanceType, err := aws.GetInstanceType()
	if err != nil {
		return utils.MakeError("Couldn't register container instance: couldn't get AWS Instance type: %s", err)
	}

	latestMetrics, errs := metrics.GetLatest()
	if len(errs) != 0 {
		return utils.MakeError("Couldn't register container instance: errors getting metrics: %+v", errs)
	}
	cpus := latestMetrics.LogicalCPUs
	memoryRemaining := latestMetrics.AvailableMemoryKB

	// Check if there's a row for us in the database already
	q := queries.NewQuerier(dbpool)
	rows, err := q.FindInstanceByName(ctx, string(instanceName))
	if err != nil {
		return utils.MakeError("RegisterInstance(): Error running query: %s", err)
	}

	// Since the `instance_id` is the primary key of `hardware.instance_info`, we
	// know that `rows` ought to contain either 0 or 1 results.
	if len(rows) == 0 {
		return utils.MakeError("RegisterInstance(): Existing row for this instance not found in the database.")
	}

	// There is an existing row in the database for this instance --- we now "take over" and update it with the correct information.
	result, err := dbpool.Exec(ctx,
		`UPDATE hardware.instance_info SET
		(instance_id, auth_token, "memoryRemainingInInstanceInMb", "CPURemainingInInstance", "GPURemainingInInstance", "maxContainers", last_pinged, ip, ami_id, location, instance_type)
		=
		($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11)
		WHERE instance_id = $1`,
		instanceName, utils.RandHex(10), memoryRemaining, 64000 /* TODO replace this with a real value */, 64000 /* TODO replace this with a real value */, cpus/2, time.Now().UTC().UnixNano(), publicIP4, amiID, region, instanceType,
	)
	if err != nil {
		return utils.MakeError("Couldn't register instance: error updating existing row in table `hardware.instance_info`: %s", err)
	}
	logger.Infof("Result of updating existing row in table `hardware.instance_info`: %v", result)
	return nil
}

func RegisterContainer() {
	// Check if container exists, etc.

}

func KillContainer() {

}

func Heartbeat() {
	// Equivalent of existing heartbeats

}

func MarkDraining() {

}

// unregisterInstance removes the row for the instance from the
// `hardware.instance_info` table.
func unregisterInstance() error {
	if dbpool == nil {
		return utils.MakeError("UnregisterInstance() called but dbdriver is not initialized!")
	}

	// Set an arbitrary deadline of 10 seconds for cleanup
	ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
	defer cancel()

	instanceName, err := aws.GetInstanceName(ctx)
	if err != nil {
		return utils.MakeError("Couldn't unregister instance: couldn't get instance name: %s", err)
	}

	q := queries.NewQuerier(dbpool)
	result, err := q.DeleteInstance(ctx, string(instanceName))
	if err != nil {
		return utils.MakeError("UnregisterInstance(): Error running delete command: %s", err)
	}
	logger.Infof("UnregisterInstance(): Output from delete command: %s", result)

	return nil
}

func SampleQuery(globalContext context.Context) {
	logger.Infof("started testquery")
	rows, err := dbpool.Query(globalContext,
		`SELECT schemaname, tablename FROM pg_catalog.pg_tables
		WHERE schemaname != $1 AND
		schemaname != $2`,
		"information_schema",
		"pg_catalog",
	)
	if err != nil {
		logger.Error(err)
		return
	}
	logger.Infof("got rows")

	defer rows.Close()

	for rows.Next() {
		var schemaname string
		var tablename string
		err = rows.Scan(&schemaname, &tablename)
		if err != nil {
			logger.Error(err)
			return
		}
		logger.Infof("schemaname: %s \t tablename: %s", schemaname, tablename)
	}

	if rows.Err() != nil {
		logger.Errorf("rows error: %s", rows.Err())
	}
}
