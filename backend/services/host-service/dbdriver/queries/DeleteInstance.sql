-- name: DeleteInstance :exec
DELETE FROM whist.instances WHERE id = pggen.arg('instanceID');
