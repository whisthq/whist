-- name: WriteInstanceStatus :exec
UPDATE cloud.instance_info
  SET status = pggen.arg('status')
  WHERE instance_name = pggen.arg('instanceName');
