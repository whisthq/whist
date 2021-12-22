package subscriptions

import "github.com/hasura/go-graphql-client"

// DeleteInstanceStatusByName deletes an instance that matches the given instance_name.
var DeleteInstanceByName struct {
	MutationResponse struct {
		AffectedRows graphql.Int `graphql:"affected_rows"`
	} `graphql:"delete_cloud_instance_info(where: {instance_name: {_eq: $instance_name}})"`
}

// UpdateInstanceStatus updates the status of an instance to the given status.
var UpdateInstanceStatus struct {
	MutationResponse struct {
		AffectedRows graphql.Int `graphql:"affected_rows"`
	} `graphql:"update_cloud_instance_info(where: {status: {_eq: $status}})"`
}
