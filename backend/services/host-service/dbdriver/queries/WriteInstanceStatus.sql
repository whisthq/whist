-- name: WriteInstanceStatus :exec
UPDATE whist.instances
  SET status = pggen.arg('status')
  WHERE id = pggen.arg('instanceID');
