// Code generated by pggen. DO NOT EDIT.

package queries

import (
	"context"
	"fmt"
	"github.com/jackc/pgconn"
	"github.com/jackc/pgx/v4"
)

const markContainerRunningSQL = `UPDATE hardware.container_info
  SET status = 'RUNNING'
  WHERE container_id = $1;`

// MarkContainerRunning implements Querier.MarkContainerRunning.
func (q *DBQuerier) MarkContainerRunning(ctx context.Context, containerID string) (pgconn.CommandTag, error) {
	ctx = context.WithValue(ctx, "pggen_query_name", "MarkContainerRunning")
	cmdTag, err := q.conn.Exec(ctx, markContainerRunningSQL, containerID)
	if err != nil {
		return cmdTag, fmt.Errorf("exec query MarkContainerRunning: %w", err)
	}
	return cmdTag, err
}

// MarkContainerRunningBatch implements Querier.MarkContainerRunningBatch.
func (q *DBQuerier) MarkContainerRunningBatch(batch *pgx.Batch, containerID string) {
	batch.Queue(markContainerRunningSQL, containerID)
}

// MarkContainerRunningScan implements Querier.MarkContainerRunningScan.
func (q *DBQuerier) MarkContainerRunningScan(results pgx.BatchResults) (pgconn.CommandTag, error) {
	cmdTag, err := results.Exec()
	if err != nil {
		return cmdTag, fmt.Errorf("exec MarkContainerRunningBatch: %w", err)
	}
	return cmdTag, err
}
