-- name: WriteMandelboxStatus :exec
UPDATE whist.mandelboxes
  SET status = pggen.arg('status')
  WHERE id = pggen.arg('mandelboxID');
