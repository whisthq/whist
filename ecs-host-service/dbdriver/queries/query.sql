-- name: FindInstanceByName :one
SELECT * from hardware.instance_info WHERE instance_name = pggen.arg('instanceName');
