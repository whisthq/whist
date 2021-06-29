// Code generated by pggen. DO NOT EDIT.

package queries

import (
	"context"
	"fmt"
	"github.com/jackc/pgtype"
	"github.com/jackc/pgx/v4"
)

const findInstanceByNameSQL = `SELECT * FROM hardware.instance_info WHERE instance_name = $1;`

type FindInstanceByNameRow struct {
	InstanceName          pgtype.Varchar `json:"instance_name"`
	CloudProviderID       pgtype.Varchar `json:"cloud_provider_id"`
	CreationTimeUtcUnixMs int            `json:"creation_time_utc_unix_ms"`
	MemoryRemainingKb     int            `json:"memory_remaining_kb"`
	NanocpusRemaining     int            `json:"nanocpus_remaining"`
	GpuVramRemainingKb    int            `json:"gpu_vram_remaining_kb"`
	MandelboxCapacity     int            `json:"mandelbox_capacity"`
	LastUpdatedUtcUnixMs  int            `json:"last_updated_utc_unix_ms"`
	Ip                    pgtype.Varchar `json:"ip"`
	AwsAmiID              pgtype.Varchar `json:"aws_ami_id"`
	Location              pgtype.Varchar `json:"location"`
	Status                pgtype.Varchar `json:"status"`
	CommitHash            pgtype.Varchar `json:"commit_hash"`
	AwsInstanceType       pgtype.Varchar `json:"aws_instance_type"`
}

// FindInstanceByName implements Querier.FindInstanceByName.
func (q *DBQuerier) FindInstanceByName(ctx context.Context, instanceName string) ([]FindInstanceByNameRow, error) {
	ctx = context.WithValue(ctx, "pggen_query_name", "FindInstanceByName")
	rows, err := q.conn.Query(ctx, findInstanceByNameSQL, instanceName)
	if err != nil {
		return nil, fmt.Errorf("query FindInstanceByName: %w", err)
	}
	defer rows.Close()
	items := []FindInstanceByNameRow{}
	for rows.Next() {
		var item FindInstanceByNameRow
		if err := rows.Scan(&item.InstanceName, &item.CloudProviderID, &item.CreationTimeUtcUnixMs, &item.MemoryRemainingKb, &item.NanocpusRemaining, &item.GpuVramRemainingKb, &item.MandelboxCapacity, &item.LastUpdatedUtcUnixMs, &item.Ip, &item.AwsAmiID, &item.Location, &item.Status, &item.CommitHash, &item.AwsInstanceType); err != nil {
			return nil, fmt.Errorf("scan FindInstanceByName row: %w", err)
		}
		items = append(items, item)
	}
	if err := rows.Err(); err != nil {
		return nil, fmt.Errorf("close FindInstanceByName rows: %w", err)
	}
	return items, err
}

// FindInstanceByNameBatch implements Querier.FindInstanceByNameBatch.
func (q *DBQuerier) FindInstanceByNameBatch(batch *pgx.Batch, instanceName string) {
	batch.Queue(findInstanceByNameSQL, instanceName)
}

// FindInstanceByNameScan implements Querier.FindInstanceByNameScan.
func (q *DBQuerier) FindInstanceByNameScan(results pgx.BatchResults) ([]FindInstanceByNameRow, error) {
	rows, err := results.Query()
	if err != nil {
		return nil, fmt.Errorf("query FindInstanceByNameBatch: %w", err)
	}
	defer rows.Close()
	items := []FindInstanceByNameRow{}
	for rows.Next() {
		var item FindInstanceByNameRow
		if err := rows.Scan(&item.InstanceName, &item.CloudProviderID, &item.CreationTimeUtcUnixMs, &item.MemoryRemainingKb, &item.NanocpusRemaining, &item.GpuVramRemainingKb, &item.MandelboxCapacity, &item.LastUpdatedUtcUnixMs, &item.Ip, &item.AwsAmiID, &item.Location, &item.Status, &item.CommitHash, &item.AwsInstanceType); err != nil {
			return nil, fmt.Errorf("scan FindInstanceByNameBatch row: %w", err)
		}
		items = append(items, item)
	}
	if err := rows.Err(); err != nil {
		return nil, fmt.Errorf("close FindInstanceByNameBatch rows: %w", err)
	}
	return items, err
}
