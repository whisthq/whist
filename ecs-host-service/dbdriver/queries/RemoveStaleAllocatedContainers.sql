-- name: RemoveStaleAllocatedContainers :exec
DELETE FROM hardware.container_info
  WHERE instance_name = pggen.arg('instanceName')
    AND status = pggen.arg('status')
    AND creation_time_utc_unix_ms < pggen.arg('creationTimeThreshold');
