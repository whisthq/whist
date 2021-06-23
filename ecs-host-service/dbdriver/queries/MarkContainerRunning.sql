-- name: MarkContainerRunning :exec
UPDATE hardware.container_info
  SET status = 'RUNNING'
  WHERE container_id = pggen.arg('containerID');
