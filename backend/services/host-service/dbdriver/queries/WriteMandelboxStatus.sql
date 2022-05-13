-- name: WriteMandelboxStatus :exec
UPDATE whist.mandelboxes
  SET (status, updated_at) = (pggen.arg('status'), pggen.arg('updated_at'))
  WHERE id = pggen.arg('mandelboxID');
