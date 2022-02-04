-- name: WriteHeartbeat :exec
UPDATE whist.instances SET (updated_at) = row(pggen.arg('updated_at')) WHERE id = pggen.arg('instanceID');
