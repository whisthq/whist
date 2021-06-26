// Code generated by pggen. DO NOT EDIT.

package queries

import (
	"context"
	"fmt"
	"github.com/jackc/pgconn"
	"github.com/jackc/pgtype"
	"github.com/jackc/pgx/v4"
)

const registerInstanceSQL = `UPDATE hardware.instance_info
  SET (cloud_provider_id, memory_remaining_kb, nanocpus_remaining, gpu_vram_remaining_kb, container_capacity, last_updated_utc_unix_ms, ip, status)
  = ($1, $2, $3, $4, $5, $6, $7, $8)
  WHERE instance_name = $9;`

type RegisterInstanceParams struct {
	CloudProviderID      pgtype.Varchar
	MemoryRemainingKB    int
	NanoCPUsRemainingKB  int
	GpuVramRemainingKb   int
	ContainerCapacity    int
	LastUpdatedUtcUnixMs int
	Ip                   pgtype.Varchar
	Status               pgtype.Varchar
	InstanceName         string
}

// RegisterInstance implements Querier.RegisterInstance.
func (q *DBQuerier) RegisterInstance(ctx context.Context, params RegisterInstanceParams) (pgconn.CommandTag, error) {
	ctx = context.WithValue(ctx, "pggen_query_name", "RegisterInstance")
	cmdTag, err := q.conn.Exec(ctx, registerInstanceSQL, params.CloudProviderID, params.MemoryRemainingKB, params.NanoCPUsRemainingKB, params.GpuVramRemainingKb, params.ContainerCapacity, params.LastUpdatedUtcUnixMs, params.Ip, params.Status, params.InstanceName)
	if err != nil {
		return cmdTag, fmt.Errorf("exec query RegisterInstance: %w", err)
	}
	return cmdTag, err
}

// RegisterInstanceBatch implements Querier.RegisterInstanceBatch.
func (q *DBQuerier) RegisterInstanceBatch(batch *pgx.Batch, params RegisterInstanceParams) {
	batch.Queue(registerInstanceSQL, params.CloudProviderID, params.MemoryRemainingKB, params.NanoCPUsRemainingKB, params.GpuVramRemainingKb, params.ContainerCapacity, params.LastUpdatedUtcUnixMs, params.Ip, params.Status, params.InstanceName)
}

// RegisterInstanceScan implements Querier.RegisterInstanceScan.
func (q *DBQuerier) RegisterInstanceScan(results pgx.BatchResults) (pgconn.CommandTag, error) {
	cmdTag, err := results.Exec()
	if err != nil {
		return cmdTag, fmt.Errorf("exec RegisterInstanceBatch: %w", err)
	}
	return cmdTag, err
}
