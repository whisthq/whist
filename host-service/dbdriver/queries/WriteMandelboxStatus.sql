-- name: WriteMandelboxStatus :exec
UPDATE hardware.mandelbox_info
  SET status = pggen.arg('status')
  WHERE mandelbox_id = pggen.arg('mandelboxID');
