package subscriptions // import "github.com/whisthq/whist/core-go/subscriptions"

import graphql "github.com/hasura/go-graphql-client"

// GraphQLQuery is a custom empty interface to
// represent the graphql queries described in this file.
type GraphQLQuery interface{}

// InstanceStatusQuery defines the event when
// the provided instance $instance_name
// changes to the provided status $status in the database
var InstanceStatusQuery struct {
	CloudInstanceInfo struct {
		InstanceName graphql.String `graphql:"instance_name"`
		Status       graphql.String `graphql:"status"`
	} `graphql:"cloud_instance_info(where: {instance_name: {_eq: $instance_name}, _and: {status: {_eq: $status}}})"`
}

// MandelboxAllocatedQuery defines the event when
// a mandelbox is assigned to the provided $instance_name
var MandelboxAllocatedQuery struct {
	CloudMandelboxInfo struct {
		InstanceName graphql.String `graphql:"instance_name"`
		MandelboxID  graphql.String `graphql:"mandelbox_id"`
		SessionID    graphql.String `graphql:"session_id"`
		UserID       graphql.String `graphql:"user_id"`
		Status       graphql.String `graphql:"status"`
	} `graphql:"cloud_mandelbox_info(where: {instance_name: {_eq: $instance_name}, _and: {status: {_eq: $status}}})"`
}

// MandelboxStatusQuery defines the event when
// a mandelbox's status changes.
var MandelboxStatusQuery struct {
	CloudMandelboxInfo struct {
		InstanceName graphql.String `graphql:"instance_name"`
		MandelboxID  graphql.String `graphql:"mandelbox_id"`
		SessionID    graphql.String `graphql:"session_id"`
		UserID       graphql.String `graphql:"user_id"`
		Status       graphql.String `graphql:"status"`
	} `graphql:"cloud_mandelbox_info(where: {status: {_eq: $status}}})"`
}
