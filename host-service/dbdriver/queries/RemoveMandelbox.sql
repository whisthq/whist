-- name: RemoveMandelbox :exec
DELETE FROM cloud.mandelbox_info WHERE mandelbox_id = pggen.arg('mandelboxID');
