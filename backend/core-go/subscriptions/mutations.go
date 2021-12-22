package subscriptions

import "github.com/hasura/go-graphql-client"

// InsertInstances inserts multiple instances to the database.
var InsertInstances struct {
	MutationResponse struct {
		AffectedRows graphql.Int `graphql:"affected_rows"`
	} `graphql:"insert_cloud_instance_info_(objects: $objects)"`
}

// InsertOneInstance inserts one instance to the database.
var InsertOneInstance struct {
	MutationResponse struct {
		AffectedRows graphql.Int `graphql:"affected_rows"`
	} `graphql:"insert_cloud_instance_info_one(objects: $objects)"`
}

// UpdateInstanceStatus updates the status of an instance to the given status.
var UpdateInstanceStatus struct {
	MutationResponse struct {
		AffectedRows graphql.Int `graphql:"affected_rows"`
	} `graphql:"update_cloud_instance_info(where: {status: {_eq: $status}})"`
}

// DeleteInstanceStatusByName deletes an instance that matches the given instance_name.
var DeleteInstanceByName struct {
	MutationResponse struct {
		AffectedRows graphql.Int `graphql:"affected_rows"`
	} `graphql:"delete_cloud_instance_info(where: {instance_name: {_eq: $instance_name}})"`
}
