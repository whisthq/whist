-- name: MarkContainerRunning :exec
UPDATE hardware.container_info
  SET status = pggen.arg('status')
  WHERE container_id = pggen.arg('containerID');
