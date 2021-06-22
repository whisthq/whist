package dbdriver // import "github.com/fractal/fractal/ecs-host-service/dbdriver"

//go:generate echo "Generating dbdriver query code..."

//go:generate -command pggen go run github.com/jschaf/pggen/cmd/pggen

// We don't pass this path in as an environment variable from the Makefile,
// since it needs to be relative to this file, not the Makefile.
//go:generate pggen gen go --schema-glob "../../main-webserver/db_migration/schema.sql" --query-glob ./queries/*.sql --go-type bigint=*int64
