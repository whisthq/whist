-- name: RemoveStaleMandelboxes :exec
DELETE FROM hardware.mandelbox_info
  WHERE
    instance_name = pggen.arg('instanceName')
    AND (
      (status = pggen.arg('allocatedStatus')
        AND creation_time_utc_unix_ms < pggen.arg('allocatedCreationTimeThreshold'))
      OR (
      (status = pggen.arg('connectingStatus')
        AND creation_time_utc_unix_ms < pggen.arg('connectingCreationTimeThreshold'))
      )
    );
