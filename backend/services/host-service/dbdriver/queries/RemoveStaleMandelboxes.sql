-- name: RemoveStaleMandelboxes :exec
DELETE FROM whist.mandelboxes
  WHERE
    instance_id = pggen.arg('instanceID')
    AND (
      (status = pggen.arg('allocatedStatus')
        AND created_at < pggen.arg('allocatedCreationTimeThreshold'))
      OR (
      (status = pggen.arg('connectingStatus')
        AND created_at < pggen.arg('connectingCreationTimeThreshold'))
      )
    );
