// Code generated by pggen. DO NOT EDIT.

package queries

import (
	"context"
	"fmt"
	"github.com/jackc/pgconn"
	"github.com/jackc/pgtype"
	"github.com/jackc/pgx/v4"
)

// Querier is a typesafe Go interface backed by SQL queries.
//
// Methods ending with Batch enqueue a query to run later in a pgx.Batch. After
// calling SendBatch on pgx.Conn, pgxpool.Pool, or pgx.Tx, use the Scan methods
// to parse the results.
type Querier interface {
	DeleteInstance(ctx context.Context, instanceID string) (pgconn.CommandTag, error)
	// DeleteInstanceBatch enqueues a DeleteInstance query into batch to be executed
	// later by the batch.
	DeleteInstanceBatch(batch genericBatch, instanceID string)
	// DeleteInstanceScan scans the result of an executed DeleteInstanceBatch query.
	DeleteInstanceScan(results pgx.BatchResults) (pgconn.CommandTag, error)

	FindInstanceByID(ctx context.Context, instanceID string) ([]FindInstanceByIDRow, error)
	// FindInstanceByIDBatch enqueues a FindInstanceByID query into batch to be executed
	// later by the batch.
	FindInstanceByIDBatch(batch genericBatch, instanceID string)
	// FindInstanceByIDScan scans the result of an executed FindInstanceByIDBatch query.
	FindInstanceByIDScan(results pgx.BatchResults) ([]FindInstanceByIDRow, error)

	FindMandelboxByID(ctx context.Context, mandelboxID string) ([]FindMandelboxByIDRow, error)
	// FindMandelboxByIDBatch enqueues a FindMandelboxByID query into batch to be executed
	// later by the batch.
	FindMandelboxByIDBatch(batch genericBatch, mandelboxID string)
	// FindMandelboxByIDScan scans the result of an executed FindMandelboxByIDBatch query.
	FindMandelboxByIDScan(results pgx.BatchResults) ([]FindMandelboxByIDRow, error)

	RegisterInstance(ctx context.Context, params RegisterInstanceParams) (pgconn.CommandTag, error)
	// RegisterInstanceBatch enqueues a RegisterInstance query into batch to be executed
	// later by the batch.
	RegisterInstanceBatch(batch genericBatch, params RegisterInstanceParams)
	// RegisterInstanceScan scans the result of an executed RegisterInstanceBatch query.
	RegisterInstanceScan(results pgx.BatchResults) (pgconn.CommandTag, error)

	RemoveMandelbox(ctx context.Context, mandelboxID string) (pgconn.CommandTag, error)
	// RemoveMandelboxBatch enqueues a RemoveMandelbox query into batch to be executed
	// later by the batch.
	RemoveMandelboxBatch(batch genericBatch, mandelboxID string)
	// RemoveMandelboxScan scans the result of an executed RemoveMandelboxBatch query.
	RemoveMandelboxScan(results pgx.BatchResults) (pgconn.CommandTag, error)

	RemoveStaleMandelboxes(ctx context.Context, params RemoveStaleMandelboxesParams) (pgconn.CommandTag, error)
	// RemoveStaleMandelboxesBatch enqueues a RemoveStaleMandelboxes query into batch to be executed
	// later by the batch.
	RemoveStaleMandelboxesBatch(batch genericBatch, params RemoveStaleMandelboxesParams)
	// RemoveStaleMandelboxesScan scans the result of an executed RemoveStaleMandelboxesBatch query.
	RemoveStaleMandelboxesScan(results pgx.BatchResults) (pgconn.CommandTag, error)

	WriteHeartbeat(ctx context.Context, updatedAt pgtype.Timestamptz, instanceID string) (pgconn.CommandTag, error)
	// WriteHeartbeatBatch enqueues a WriteHeartbeat query into batch to be executed
	// later by the batch.
	WriteHeartbeatBatch(batch genericBatch, updatedAt pgtype.Timestamptz, instanceID string)
	// WriteHeartbeatScan scans the result of an executed WriteHeartbeatBatch query.
	WriteHeartbeatScan(results pgx.BatchResults) (pgconn.CommandTag, error)


	WriteInstanceStatus(ctx context.Context, status pgtype.Varchar, instanceID string) (pgconn.CommandTag, error)
	// WriteInstanceStatusBatch enqueues a WriteInstanceStatus query into batch to be executed
	// later by the batch.
	WriteInstanceStatusBatch(batch genericBatch, status pgtype.Varchar, instanceID string)

	// WriteInstanceStatusScan scans the result of an executed WriteInstanceStatusBatch query.
	WriteInstanceStatusScan(results pgx.BatchResults) (pgconn.CommandTag, error)

	WriteMandelboxStatus(ctx context.Context, status pgtype.Varchar, mandelboxID string) (pgconn.CommandTag, error)
	// WriteMandelboxStatusBatch enqueues a WriteMandelboxStatus query into batch to be executed
	// later by the batch.
	WriteMandelboxStatusBatch(batch genericBatch, status pgtype.Varchar, mandelboxID string)
	// WriteMandelboxStatusScan scans the result of an executed WriteMandelboxStatusBatch query.
	WriteMandelboxStatusScan(results pgx.BatchResults) (pgconn.CommandTag, error)
}

type DBQuerier struct {
	conn  genericConn   // underlying Postgres transport to use
	types *typeResolver // resolve types by name
}

var _ Querier = &DBQuerier{}

// genericConn is a connection to a Postgres database. This is usually backed by
// *pgx.Conn, pgx.Tx, or *pgxpool.Pool.
type genericConn interface {
	// Query executes sql with args. If there is an error the returned Rows will
	// be returned in an error state. So it is allowed to ignore the error
	// returned from Query and handle it in Rows.
	Query(ctx context.Context, sql string, args ...interface{}) (pgx.Rows, error)

	// QueryRow is a convenience wrapper over Query. Any error that occurs while
	// querying is deferred until calling Scan on the returned Row. That Row will
	// error with pgx.ErrNoRows if no rows are returned.
	QueryRow(ctx context.Context, sql string, args ...interface{}) pgx.Row

	// Exec executes sql. sql can be either a prepared statement name or an SQL
	// string. arguments should be referenced positionally from the sql string
	// as $1, $2, etc.
	Exec(ctx context.Context, sql string, arguments ...interface{}) (pgconn.CommandTag, error)
}

// genericBatch batches queries to send in a single network request to a
// Postgres server. This is usually backed by *pgx.Batch.
type genericBatch interface {
	// Queue queues a query to batch b. query can be an SQL query or the name of a
	// prepared statement. See Queue on *pgx.Batch.
	Queue(query string, arguments ...interface{})
}

// NewQuerier creates a DBQuerier that implements Querier. conn is typically
// *pgx.Conn, pgx.Tx, or *pgxpool.Pool.
func NewQuerier(conn genericConn) *DBQuerier {
	return NewQuerierConfig(conn, QuerierConfig{})
}

type QuerierConfig struct {
	// DataTypes contains pgtype.Value to use for encoding and decoding instead
	// of pggen-generated pgtype.ValueTranscoder.
	//
	// If OIDs are available for an input parameter type and all of its
	// transitive dependencies, pggen will use the binary encoding format for
	// the input parameter.
	DataTypes []pgtype.DataType
}

// NewQuerierConfig creates a DBQuerier that implements Querier with the given
// config. conn is typically *pgx.Conn, pgx.Tx, or *pgxpool.Pool.
func NewQuerierConfig(conn genericConn, cfg QuerierConfig) *DBQuerier {
	return &DBQuerier{conn: conn, types: newTypeResolver(cfg.DataTypes)}
}

// WithTx creates a new DBQuerier that uses the transaction to run all queries.
func (q *DBQuerier) WithTx(tx pgx.Tx) (*DBQuerier, error) {
	return &DBQuerier{conn: tx}, nil
}

// preparer is any Postgres connection transport that provides a way to prepare
// a statement, most commonly *pgx.Conn.
type preparer interface {
	Prepare(ctx context.Context, name, sql string) (sd *pgconn.StatementDescription, err error)
}

// PrepareAllQueries executes a PREPARE statement for all pggen generated SQL
// queries in querier files. Typical usage is as the AfterConnect callback
// for pgxpool.Config
//
// pgx will use the prepared statement if available. Calling PrepareAllQueries
// is an optional optimization to avoid a network round-trip the first time pgx
// runs a query if pgx statement caching is enabled.
func PrepareAllQueries(ctx context.Context, p preparer) error {
	if _, err := p.Prepare(ctx, deleteInstanceSQL, deleteInstanceSQL); err != nil {
		return fmt.Errorf("prepare query 'DeleteInstance': %w", err)
	}
	if _, err := p.Prepare(ctx, findInstanceByIDSQL, findInstanceByIDSQL); err != nil {
		return fmt.Errorf("prepare query 'FindInstanceByID': %w", err)
	}
	if _, err := p.Prepare(ctx, findMandelboxByIDSQL, findMandelboxByIDSQL); err != nil {
		return fmt.Errorf("prepare query 'FindMandelboxByID': %w", err)
	}
	if _, err := p.Prepare(ctx, registerInstanceSQL, registerInstanceSQL); err != nil {
		return fmt.Errorf("prepare query 'RegisterInstance': %w", err)
	}
	if _, err := p.Prepare(ctx, removeMandelboxSQL, removeMandelboxSQL); err != nil {
		return fmt.Errorf("prepare query 'RemoveMandelbox': %w", err)
	}
	if _, err := p.Prepare(ctx, removeStaleMandelboxesSQL, removeStaleMandelboxesSQL); err != nil {
		return fmt.Errorf("prepare query 'RemoveStaleMandelboxes': %w", err)
	}
	if _, err := p.Prepare(ctx, writeHeartbeatSQL, writeHeartbeatSQL); err != nil {
		return fmt.Errorf("prepare query 'WriteHeartbeat': %w", err)
	}
	if _, err := p.Prepare(ctx, writeInstanceStatusSQL, writeInstanceStatusSQL); err != nil {
		return fmt.Errorf("prepare query 'WriteInstanceStatus': %w", err)
	}
	if _, err := p.Prepare(ctx, writeMandelboxStatusSQL, writeMandelboxStatusSQL); err != nil {
		return fmt.Errorf("prepare query 'WriteMandelboxStatus': %w", err)
	}
	return nil
}

// typeResolver looks up the pgtype.ValueTranscoder by Postgres type name.
type typeResolver struct {
	connInfo *pgtype.ConnInfo // types by Postgres type name
}

func newTypeResolver(types []pgtype.DataType) *typeResolver {
	ci := pgtype.NewConnInfo()
	for _, typ := range types {
		if txt, ok := typ.Value.(textPreferrer); ok && typ.OID != unknownOID {
			typ.Value = txt.ValueTranscoder
		}
		ci.RegisterDataType(typ)
	}
	return &typeResolver{connInfo: ci}
}

// findValue find the OID, and pgtype.ValueTranscoder for a Postgres type name.
func (tr *typeResolver) findValue(name string) (uint32, pgtype.ValueTranscoder, bool) {
	typ, ok := tr.connInfo.DataTypeForName(name)
	if !ok {
		return 0, nil, false
	}
	v := pgtype.NewValue(typ.Value)
	return typ.OID, v.(pgtype.ValueTranscoder), true
}

// setValue sets the value of a ValueTranscoder to a value that should always
// work and panics if it fails.
func (tr *typeResolver) setValue(vt pgtype.ValueTranscoder, val interface{}) pgtype.ValueTranscoder {
	if err := vt.Set(val); err != nil {
		panic(fmt.Sprintf("set ValueTranscoder %T to %+v: %s", vt, val, err))
	}
	return vt
}

const deleteInstanceSQL = `DELETE FROM whist.instances WHERE id = $1;`

// DeleteInstance implements Querier.DeleteInstance.
func (q *DBQuerier) DeleteInstance(ctx context.Context, instanceID string) (pgconn.CommandTag, error) {
	ctx = context.WithValue(ctx, "pggen_query_name", "DeleteInstance")
	cmdTag, err := q.conn.Exec(ctx, deleteInstanceSQL, instanceID)
	if err != nil {
		return cmdTag, fmt.Errorf("exec query DeleteInstance: %w", err)
	}
	return cmdTag, err
}

// DeleteInstanceBatch implements Querier.DeleteInstanceBatch.
func (q *DBQuerier) DeleteInstanceBatch(batch genericBatch, instanceID string) {
	batch.Queue(deleteInstanceSQL, instanceID)
}

// DeleteInstanceScan implements Querier.DeleteInstanceScan.
func (q *DBQuerier) DeleteInstanceScan(results pgx.BatchResults) (pgconn.CommandTag, error) {
	cmdTag, err := results.Exec()
	if err != nil {
		return cmdTag, fmt.Errorf("exec DeleteInstanceBatch: %w", err)
	}
	return cmdTag, err
}

// textPreferrer wraps a pgtype.ValueTranscoder and sets the preferred encoding
// format to text instead binary (the default). pggen uses the text format
// when the OID is unknownOID because the binary format requires the OID.
// Typically occurs if the results from QueryAllDataTypes aren't passed to
// NewQuerierConfig.
type textPreferrer struct {
	pgtype.ValueTranscoder
	typeName string
}

// PreferredParamFormat implements pgtype.ParamFormatPreferrer.
func (t textPreferrer) PreferredParamFormat() int16 { return pgtype.TextFormatCode }

func (t textPreferrer) NewTypeValue() pgtype.Value {
	return textPreferrer{pgtype.NewValue(t.ValueTranscoder).(pgtype.ValueTranscoder), t.typeName}
}

func (t textPreferrer) TypeName() string {
	return t.typeName
}

// unknownOID means we don't know the OID for a type. This is okay for decoding
// because pgx call DecodeText or DecodeBinary without requiring the OID. For
// encoding parameters, pggen uses textPreferrer if the OID is unknown.
const unknownOID = 0
