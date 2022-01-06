package subscriptions // import "github.com/whisthq/whist/backend/services/subscriptions"

import (
	graphql "github.com/hasura/go-graphql-client"
)

// GraphQLQuery is a custom empty interface to represent the graphql queries
// described in this file. An advantage is that these queries can be used both
// as subscriptions and normal queries.
type GraphQLQuery interface{}

// WhistInstances is the mapping of the `whist.hosts` table. It is meant to be used
// on the database subscriptions.
type WhistInstances []struct {
	ID        graphql.String `graphql:"id"`
	Provider  graphql.String `graphql:"provider"`
	Region    graphql.String `graphql:"region"`
	ImageID   graphql.String `graphql:"image_id"`
	ClientSHA graphql.String `graphql:"client_sha"`
	IPAddress graphql.String `graphql:"ip_addr"`
	Capacity  graphql.Int    `graphql:"capacity"`
	Status    graphql.String `graphql:"status"`
	CreatedAt graphql.String `graphql:"created_at"`
	UpdatedAt graphql.String `graphql:"updated_at"`
}

// WhistMandelboxes is the mapping of the `whist.mandelboxes` table. It is meant to be used
// on the database subscriptions.
type WhistMandelboxes []struct {
	ID        graphql.String `graphql:"id"`
	App       graphql.String `graphql:"app"`
	HostID    graphql.String `graphql:"instance_id"`
	UserID    graphql.String `graphql:"user_id"`
	SessionID graphql.String `graphql:"session_id"`
	CreatedAt graphql.String `graphql:"created_at"`
	Status    graphql.String `graphql:"status"`
}

// WhistImages is the mapping of the `whist.images` table. It is meant to be used
// on the database subscriptions.
type WhistImages []struct {
	Provider  graphql.String `graphql:"provider"`
	Region    graphql.String `graphql:"region"`
	ImageID   graphql.String `graphql:"image_id"`
	ClientSHA graphql.String `graphql:"client_sha"`
	UpdatedAt graphql.String `graphql:"updated_at"`
}

// QueryInstanceByIdWithStatus returns an instance that matches the given instance_name and status.
var QueryInstanceByIdWithStatus struct {
	WhistInstances `graphql:"whist_hosts(where: {id: {_eq: $id}, _and: {status: {_eq: $status}}})"`
}

// QueryInstanceById returns an instance that matches the given instance_name and status.
var QueryInstanceById struct {
	WhistInstances `graphql:"whist_hosts(where: {id: {_eq: $id}})"`
}

// QueryInstanceStatus returns any instance that matches the given status.
var QueryInstancesByStatus struct {
	WhistInstances `graphql:"whist_hosts(where: {status: {_eq: $status}})"`
}

// QueryFreeInstances returns any instance that matches the given status.
var QueryFreeInstances struct {
	WhistInstances `graphql:"whist_hosts(where: {capacity: {_eq: $capacity}, _and {status: {_eq: $status}}})"`
}

// QueryMandelboxesById returns a mandelbox associated with the given instance
// and that has the given status.
var QueryMandelboxesByInstanceId struct {
	WhistMandelboxes `graphql:"whist_mandelboxes(where: {instance_id: {_eq: $instance_id}, _and: {status: {_eq: $status}}})"`
}

// QueryMandelboxStatus returns every mandelbox that matches the given status.
var QueryMandelboxByStatus struct {
	WhistMandelboxes `graphql:"whist_mandelboxes(where: {status: {_eq: $status}})"`
}
