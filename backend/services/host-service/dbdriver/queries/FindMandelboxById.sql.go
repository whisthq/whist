// Code generated by pggen. DO NOT EDIT.

package queries

import (
	"context"
	"fmt"
	"github.com/jackc/pgtype"
	"github.com/jackc/pgx/v4"
)

const findMandelboxByIDSQL = `SELECT * FROM whist.mandelboxes
  WHERE id = $1;`

type FindMandelboxByIDRow struct {
	ID         pgtype.Varchar     `json:"id"`
	App        pgtype.Varchar     `json:"app"`
	InstanceID pgtype.Varchar     `json:"instance_id"`
	UserID     pgtype.Varchar     `json:"user_id"`
	SessionID  pgtype.Varchar     `json:"session_id"`
	Status     pgtype.Varchar     `json:"status"`
	CreatedAt  pgtype.Timestamptz `json:"created_at"`
	UpdatedAt  pgtype.Timestamptz `json:"updated_at"`
}

// FindMandelboxByID implements Querier.FindMandelboxByID.
func (q *DBQuerier) FindMandelboxByID(ctx context.Context, mandelboxID string) ([]FindMandelboxByIDRow, error) {
	ctx = context.WithValue(ctx, "pggen_query_name", "FindMandelboxByID")
	rows, err := q.conn.Query(ctx, findMandelboxByIDSQL, mandelboxID)
	if err != nil {
		return nil, fmt.Errorf("query FindMandelboxByID: %w", err)
	}
	defer rows.Close()
	items := []FindMandelboxByIDRow{}
	for rows.Next() {
		var item FindMandelboxByIDRow
		if err := rows.Scan(&item.ID, &item.App, &item.InstanceID, &item.UserID, &item.SessionID, &item.Status, &item.CreatedAt, &item.UpdatedAt); err != nil {
			return nil, fmt.Errorf("scan FindMandelboxByID row: %w", err)
		}
		items = append(items, item)
	}
	if err := rows.Err(); err != nil {
		return nil, fmt.Errorf("close FindMandelboxByID rows: %w", err)
	}
	return items, err
}

// FindMandelboxByIDBatch implements Querier.FindMandelboxByIDBatch.
func (q *DBQuerier) FindMandelboxByIDBatch(batch genericBatch, mandelboxID string) {
	batch.Queue(findMandelboxByIDSQL, mandelboxID)
}

// FindMandelboxByIDScan implements Querier.FindMandelboxByIDScan.
func (q *DBQuerier) FindMandelboxByIDScan(results pgx.BatchResults) ([]FindMandelboxByIDRow, error) {
	rows, err := results.Query()
	if err != nil {
		return nil, fmt.Errorf("query FindMandelboxByIDBatch: %w", err)
	}
	defer rows.Close()
	items := []FindMandelboxByIDRow{}
	for rows.Next() {
		var item FindMandelboxByIDRow
		if err := rows.Scan(&item.ID, &item.App, &item.InstanceID, &item.UserID, &item.SessionID, &item.Status, &item.CreatedAt, &item.UpdatedAt); err != nil {
			return nil, fmt.Errorf("scan FindMandelboxByIDBatch row: %w", err)
		}
		items = append(items, item)
	}
	if err := rows.Err(); err != nil {
		return nil, fmt.Errorf("close FindMandelboxByIDBatch rows: %w", err)
	}
	return items, err
}
