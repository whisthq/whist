-- name: DeleteInstance :exec
DELETE FROM cloud.instances WHERE id = pggen.arg('instanceId');
