package dbdriver // import "github.com/fractal/fractal/ecs-host-service/dbdriver"

import (
	"context"
	"strconv"
	"sync"
	"time"

	"github.com/jackc/pgx/v4/pgxpool"

	logger "github.com/fractal/fractal/ecs-host-service/fractallogger"
	"github.com/fractal/fractal/ecs-host-service/utils"
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

// If requireExistingRow is true, then the host service will return an error if
// the database doesn't already contain a row for it (i.e. the webserver wasn't
// expecting this host to spin up).
// TODO: add environment variable configuration for that
func RegisterInstance(ctx context.Context, requireExistingRow bool) error {
	if dbpool == nil {
		return logger.MakeError("RegisterInstance() called but dbdriver is not initialized!")
	}

	instanceName, err := logger.GetInstanceName()
	if err != nil {
		return logger.MakeError("Couldn't register instance: couldn't get instance name: %s", err)
	}
	publicIP4, err := logger.GetAwsPublicIpv4()
	if err != nil {
		return logger.MakeError("Couldn't register instance: couldn't get public IPv4: %s", err)
	}
	amiID, err := logger.GetAwsAmiID()
	if err != nil {
		return logger.MakeError("Couldn't register instance: couldn't get AMI ID: %s", err)
	}
	region, err := logger.GetAwsPlacementRegion()
	if err != nil {
		return logger.MakeError("Couldn't register instance: couldn't get AWS Placement Region: %s", err)
	}
	instanceType, err := logger.GetAwsInstanceType()
	if err != nil {
		return logger.MakeError("Couldn't register container instance: couldn't get AWS Instance type: %s", err)
	}
	cpustr, err := logger.GetNumLogicalCPUs()
	if err != nil {
		return logger.MakeError("Couldn't register container instance: couldn't get number of logical CPUs: %s", err)
	}
	cpus, err := strconv.Atoi(cpustr)
	if err != nil {
		return logger.MakeError("Couldn't register container instance: couldn't get number of logical CPUs: %s", err)
	}
	memoryRemaining, err := logger.GetAvailableMemoryInKB()
	if err != nil {
		return logger.MakeError("Couldn't register container instance: couldn't get amount of memory remaining: %s", err)
	}

	// Check if there's a row for us in the database already
	// TODO: factor our `hardware.instance_info` into a variable
	rows, err := dbpool.Query(ctx,
		`SELECT * FROM hardware.instance_info
		WHERE instance_id = $1`,
		instanceName,
	)
	if err != nil {
		return logger.MakeError("RegisterInstance(): Error running query: %s", err)
	}
	defer rows.Close()

	// Since the `instance_id` is the primary key of `hardware.instance_info`, we
	// know that `rows` will contain either 0 or 1 results.
	if !rows.Next() {
		if requireExistingRow {
			return logger.MakeError("RegisterInstance(): Existing row for this instance not found in the database, but `requireExistingRow` set to `true`.")
		}

		// We want to add a row for this instance.
		result, err := dbpool.Exec(ctx,
			`INSERT INTO hardware.instance_info
			(instance_id, auth_token, created_at, "memoryRemainingInInstanceInMb", "CPURemainingInInstance", "GPURemainingInInstance", "maxContainers", last_pinged, ip, ami_id, location, instance_type)
			VALUES
			($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12)`,
			instanceName, utils.RandHex(10), time.Now().UTC().UnixNano(), memoryRemaining, 64000 /* TODO replace this with a real value */, 64000 /* TODO replace this with a real value */, cpus/2, time.Now().UTC().UnixNano(), publicIP4, amiID, region, instanceType,
			// TODO: milliseconds or nanoseconds?
			// TODO: divide memory remaining?
		// TODO: switch some of these field names, and bug Leor about it
		)
		if err != nil {
			return logger.MakeError("Couldn't register instance: error inserting new row into table `hardware.instance_info`: %s", err)
		}
		logger.Infof("Result of inserting new row into table `hardware.instance_info`: %v", result)
		return nil
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
		return logger.MakeError("Couldn't register instance: error updating existing row in table `hardware.instance_info`: %s", err)
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

// UnregisterInstance removes the row for the instance from the
// `hardware.instance_info` table.
func UnregisterInstance(ctx context.Context) error {
	if dbpool == nil {
		return logger.MakeError("UnregisterInstance() called but dbdriver is not initialized!")
	}

	instanceName, err := logger.GetInstanceName()
	if err != nil {
		return logger.MakeError("Couldn't unregister instance: couldn't get instance name: %s", err)
	}

	result, err := dbpool.Exec(ctx,
		`DELETE FROM hardware.instance_info
		WHERE instance_id = $1`,
		instanceName,
	)
	if err != nil {
		return logger.MakeError("UnregisterInstance(): Error running delete command: %s", err)
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
