-- name: FindMandelboxesForUser :many
SELECT * FROM hardware.mandelbox_info
  WHERE instance_name = pggen.arg('instanceName')
    AND user_id = pggen.arg('userID');
