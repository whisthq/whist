-- name: VerifyAllocatedContainer :many
SELECT * FROM hardware.container_info
  WHERE instance_name = pggen.arg('instanceName')
    AND user_id = pggen.arg('userID');
