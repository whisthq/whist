package subscriptions

import "github.com/hasura/go-graphql-client"

// InsertInstances inserts multiple instances to the database.
var InsertInstances struct {
	MutationResponse struct {
		AffectedRows graphql.Int `graphql:"affected_rows"`
	} `graphql:"insert_whist_hosts(objects: $objects)"`
}

// InsertOneInstance inserts one instance to the database.
var InsertOneInstance struct {
	MutationResponse struct {
		AffectedRows graphql.Int `graphql:"affected_rows"`
	} `graphql:"insert_whist_hosts_one(objects: $objects)"`
}

// UpdateInstanceStatus updates the status of an instance to the given status.
var UpdateInstanceStatus struct {
	MutationResponse struct {
		AffectedRows graphql.Int `graphql:"affected_rows"`
	} `graphql:"update_whist_hosts(where: {status: {_eq: $status}})"`
}

// DeleteInstanceStatusById deletes an instance that matches the given instance_name.
var DeleteInstanceById struct {
	MutationResponse struct {
		AffectedRows graphql.Int `graphql:"affected_rows"`
	} `graphql:"delete_whist_hosts(where: {id: {_eq: $id}})"`
}
