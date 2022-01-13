-- name: FindMandelboxById :many
SELECT * FROM whist.mandelboxes
  WHERE id = pggen.arg('mandelboxId');
