package subscriptions

import "github.com/hasura/go-graphql-client"

// DeleteInstanceStatusByName deletes an instance that matches the given instance_name.
var DeleteInstanceByName struct {
	MutationResponse struct {
		AffectedRows graphql.Int `graphql:"affected_rows"`
	} `graphql:"delete_cloud_instance_info(where: {instance_name: {_eq: $instance_name}})"`
}
