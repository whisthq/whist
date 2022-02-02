package subscriptions

import "github.com/hasura/go-graphql-client"

// InsertInstances inserts multiple instances to the database.
var InsertInstances struct {
	MutationResponse struct {
		AffectedRows graphql.Int `graphql:"affected_rows"`
	} `graphql:"insert_whist_instances(objects: $objects)"`
}

// InsertOneInstance inserts one instance to the database.
var InsertOneInstance struct {
	MutationResponse struct {
		AffectedRows graphql.Int `graphql:"affected_rows"`
	} `graphql:"insert_whist_instances_one(objects: $objects)"`
}

// UpdateInstanceStatus updates the status of an instance to the given status.
var UpdateInstanceStatus struct {
	MutationResponse struct {
		AffectedRows graphql.Int `graphql:"affected_rows"`
	} `graphql:"update_whist_instances(where: {id: {_eq: $id}}, _set: {status: $status})"`
}

// DeleteInstanceStatusById deletes an instance that matches the given instance_name.
var DeleteInstanceById struct {
	MutationResponse struct {
		AffectedRows graphql.Int `graphql:"affected_rows"`
	} `graphql:"delete_whist_instances(where: {id: {_eq: $id}})"`
}

// InsertOneImage inserts multiple images to the database.
var InsertImages struct {
	MutationResponse struct {
		AffectedRows graphql.Int `graphql:"affected_rows"`
	} `graphql:"insert_whist_images(objects: $objects)"`
}

// UpdateImage updates the row of an image to the given values.
var UpdateImage struct {
	MutationResponse struct {
		AffectedRows graphql.Int `graphql:"affected_rows"`
	} `graphql:"update_whist_images(where: {region: {_eq: $region}, _and: {provider: {_eq: $provider}}}, _set: {client_sha: $client_sha, image_id: $image_id, provider: $provider, region: $region, updated_at: $updated_at})"`
}
