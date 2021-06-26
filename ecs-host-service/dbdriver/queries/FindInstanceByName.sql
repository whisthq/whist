-- name: FindInstanceByName :many
SELECT * FROM hardware.instance_info WHERE instance_name = pggen.arg('instanceName');
