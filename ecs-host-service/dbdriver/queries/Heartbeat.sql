-- name: Heartbeat :exec
UPDATE hardware.instance_info SET
  (memory_remaining_kb, nanocpus_remaining, gpu_vram_remaining_kb, last_updated_utc_unix_ms) =
  (pggen.arg('memoryRemainingKB'), pggen.arg('nanoCPUsRemainingKB'), pggen.arg('gpuVramRemainingKb'), pggen.arg('lastUpdatedUtcUnixMs'))
  WHERE instance_name = pggen.arg('instanceName');
