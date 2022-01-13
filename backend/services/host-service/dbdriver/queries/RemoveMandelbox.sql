-- name: RemoveMandelbox :exec
DELETE FROM whist.mandelboxes WHERE id = pggen.arg('mandelboxId');
