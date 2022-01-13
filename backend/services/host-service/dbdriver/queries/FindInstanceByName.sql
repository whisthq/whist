-- name: FindInstanceByName :many
SELECT * FROM whist.instances WHERE id = pggen.arg('instanceID');
