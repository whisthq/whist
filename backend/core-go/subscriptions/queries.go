package subscriptions // import "github.com/whisthq/whist/backend/core-go/subscriptions"

import graphql "github.com/hasura/go-graphql-client"

// GraphQLQuery is a custom empty interface to represent the graphql queries
// described in this file. An advantage is that these queries can be used both
// as subscriptions and normal queries.
type GraphQLQuery interface{}

// QueryInstanceStatus returns an instance that matches the given instance_name and status.
var QueryInstanceStatus struct {
	CloudInstanceInfo struct {
		InstanceName graphql.String `graphql:"instance_name"`
		Status       graphql.String `graphql:"status"`
	} `graphql:"cloud_instance_info(where: {instance_name: {_eq: $instance_name}, _and: {status: {_eq: $status}}})"`
}

// QueryMandelboxesByInstanceName returns a mandelbox associated with the given instance
// and that is on the given status.
var QueryMandelboxesByInstanceName struct {
	CloudMandelboxInfo struct {
		InstanceName graphql.String `graphql:"instance_name"`
		MandelboxID  graphql.String `graphql:"mandelbox_id"`
		SessionID    graphql.String `graphql:"session_id"`
		UserID       graphql.String `graphql:"user_id"`
		Status       graphql.String `graphql:"status"`
	} `graphql:"cloud_mandelbox_info(where: {instance_name: {_eq: $instance_name}, _and: {status: {_eq: $status}}})"`
}

// QueryMandelboxStatus returns every mandelbox that matches the given status.
var QueryMandelboxStatus struct {
	CloudMandelboxInfo struct {
		InstanceName graphql.String `graphql:"instance_name"`
		MandelboxID  graphql.String `graphql:"mandelbox_id"`
		SessionID    graphql.String `graphql:"session_id"`
		UserID       graphql.String `graphql:"user_id"`
		Status       graphql.String `graphql:"status"`
	} `graphql:"cloud_mandelbox_info(where: {status: {_eq: $status}})"`
}
