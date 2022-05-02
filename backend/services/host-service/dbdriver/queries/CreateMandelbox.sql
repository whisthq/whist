-- name: CreateMandelbox :exec
INSERT INTO whist.mandelboxes (id, app, instance_id, user_id, session_id, status, created_at) 
VALUES(pggen.arg('id'), pggen.arg('app'), pggen.arg('instance_id'), pggen.arg('user_id'), pggen.arg('session_id'), pggen.arg('status'), pggen.arg('created_at'));