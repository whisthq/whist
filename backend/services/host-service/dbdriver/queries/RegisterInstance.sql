-- name: RegisterInstance :exec
UPDATE whist.instances
  SET (provider, region, image_id, client_sha, ip_addr, instance_type, remaining_capacity, status, created_at, updated_at)
  = (pggen.arg('provider'), pggen.arg('region'), pggen.arg('image_id'), pggen.arg('client_sha'), pggen.arg('ip_addr'), pggen.arg('instance_type'), pggen.arg('remaining_capacity'), pggen.arg('status'), pggen.arg('created_at'), pggen.arg('updated_at'))
  WHERE id = pggen.arg('instanceID');
