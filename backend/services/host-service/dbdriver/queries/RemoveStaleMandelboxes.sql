-- name: RemoveStaleMandelboxes :exec
DELETE FROM whist.mandelboxes
  WHERE
    instance_id = pggen.arg('instanceID')
    AND (
      (status = pggen.arg('allocatedStatus')
        AND updated_at < pggen.arg('allocatedCreationTimeThreshold'))
      OR (
      (status = pggen.arg('connectingStatus')
        AND updated_at < pggen.arg('connectingCreationTimeThreshold'))
      )
    );
