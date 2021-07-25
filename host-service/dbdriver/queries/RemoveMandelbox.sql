-- name: RemoveMandelbox :exec
DELETE FROM hardware.mandelbox_info WHERE mandelbox_id = pggen.arg('mandelboxID');
