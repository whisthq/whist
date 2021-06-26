-- name: RemoveContainer :exec
DELETE FROM hardware.container_info WHERE container_id = pggen.arg('containerID');
