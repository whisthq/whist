package dbdriver // import "github.com/fractal/fractal/ecs-host-service/dbdriver"

import (
	"context"
	"strings"
	"time"

	"github.com/jackc/pgtype"

	logger "github.com/fractal/fractal/ecs-host-service/fractallogger"
	"github.com/fractal/fractal/ecs-host-service/metadata"
	"github.com/fractal/fractal/ecs-host-service/metadata/aws"
	"github.com/fractal/fractal/ecs-host-service/metrics"
	"github.com/fractal/fractal/ecs-host-service/utils"

	"github.com/fractal/fractal/ecs-host-service/dbdriver/queries"
)

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
	instanceID, err := aws.GetInstanceID()
	if err != nil {
		return utils.MakeError("Couldn't register container instance: couldn't get AWS Instance id: %s", err)
	}

	latestMetrics, errs := metrics.GetLatest()
	if len(errs) != 0 {
		return utils.MakeError("Couldn't register container instance: errors getting metrics: %+v", errs)
	}

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

	// Verify that the properties of the existing row are actually as we expect
	if rows[0].AwsAmiID.String != string(amiID) {
		return utils.MakeError("RegisterInstance(): Existing database row found, but AMI differs. Expected %s, Got %s", amiID, rows[0].AwsAmiID.String)
	}
	if rows[0].Location.String != string(region) {
		return utils.MakeError("RegisterInstance(): Existing database row found, but location differs. Expected %s, Got %s", region, rows[0].Location.String)
	}
	if rows[0].CommitHash.Status != pgtype.Present || strings.HasPrefix(metadata.GetGitCommit(), rows[0].CommitHash.String) {
		// This is the only string where we have to check status, since an empty string is a prefix for anything.
		return utils.MakeError("RegisterInstance(): Existing database row found, but commit hash differs. Expected %s, Got %s", metadata.GetGitCommit(), rows[0].CommitHash.String)
	}
	if rows[0].AwsInstanceType.String != string(instanceType) {
		return utils.MakeError("RegisterInstance(): Existing database row found, but AWS instance type differs. Expected %s, Got %s", instanceType, rows[0].AwsInstanceType.String)
	}
	// TODO: factor out pre-connection
	if rows[0].Status.String != "PRE-CONNECTION" {
		return utils.MakeError("RegisterInstance(): Existing database row found, but status differs. Expected %s, Got %s", "PRE-CONNECTION", rows[0].Status.String)
	}

	// There is an existing row in the database for this instance --- we now "take over" and update it with the correct information.
	result, err := q.RegisterInstance(ctx, queries.RegisterInstanceParams{
		CloudProviderID: pgtype.Varchar{
			String: "aws-" + string(instanceID),
			Status: pgtype.Present,
		},
		MemoryRemainingKB:    int(latestMetrics.AvailableMemoryKB),
		NanoCPUsRemainingKB:  int(latestMetrics.NanoCPUsRemaining),
		GpuVramRemainingKb:   int(latestMetrics.FreeVideoMemoryKB),
		ContainerCapacity:    4 * latestMetrics.NumberOfGPUs,
		LastUpdatedUtcUnixMs: int(time.Now().UnixNano() / 1000),
		Ip: pgtype.Varchar{
			String: publicIP4.String(),
			Status: pgtype.Present,
		},
		Status: pgtype.Varchar{
			String: "ACTIVE",
			Status: pgtype.Present,
		},
		InstanceName: string(instanceName),
	})
	if err != nil {
		return utils.MakeError("Couldn't register instance: error updating existing row in table `hardware.instance_info`: %s", err)
	}
	logger.Infof("Result of updating existing row in table `hardware.instance_info`: %v", result)
	return nil
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
