-- name: UpdateInstanceCapacity :exec
UPDATE whist.instances SET (remaining_capacity) = row(remaining_capacity + pggen.arg('removed_mandelboxes')) WHERE id = pggen.arg('instanceID');
