-- name: WriteHeartbeat :exec
UPDATE whist.instances SET (updated_at) = (pggen.arg('updated_at')) WHERE id = pggen.arg('instanceId');
