-- name: DeleteInstance :exec
DELETE FROM hardware.instance_info WHERE instance_name = pggen.arg('instanceName');
