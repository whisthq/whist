-- name: FindMandelboxById :many
SELECT * FROM cloud.mandelbox_info
  WHERE mandelbox_id = pggen.arg('mandelboxID');
