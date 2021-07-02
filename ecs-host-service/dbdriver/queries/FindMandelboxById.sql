-- name: FindMandelboxById :many
SELECT * FROM hardware.mandelbox_info
  WHERE mandelbox_id = pggen.arg('mandelboxID');
