-- name: DeleteInstance :exec
DELETE FROM cloud.instance_info WHERE instance_name = pggen.arg('instanceName');
