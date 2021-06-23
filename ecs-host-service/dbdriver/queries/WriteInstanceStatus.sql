-- name: WriteInstanceStatus :exec
UPDATE hardware.instance_info
  SET status = pggen.arg('status')
  WHERE instance_name = pggen.arg('instanceName');
