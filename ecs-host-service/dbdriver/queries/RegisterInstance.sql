-- name: RegisterInstance :exec
UPDATE hardware.instance_info
  SET (cloud_provider_id, memory_remaining_kb, nanocpus_remaining, gpu_vram_remaining_kb, container_capacity, last_updated_utc_unix_ms, ip, status)
  = (pggen.arg('cloudProviderID'), pggen.arg('memoryRemainingKB'), pggen.arg('nanoCPUsRemainingKB'), pggen.arg('gpuVramRemainingKb'), pggen.arg('containerCapacity'), pggen.arg('lastUpdatedUtcUnixMs'), pggen.arg('ip'), pggen.arg('status'))
  WHERE instance_name = pggen.arg('instanceName');
