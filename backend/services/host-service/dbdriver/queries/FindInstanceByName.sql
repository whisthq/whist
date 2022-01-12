-- name: FindInstanceByName :many
SELECT * FROM cloud.instance_info WHERE instance_name = pggen.arg('instanceName');
