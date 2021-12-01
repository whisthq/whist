package subscriptions // import "github.com/fractal/whist/core-go/subscriptions"

import graphql "github.com/hasura/go-graphql-client"

type GraphQLQuery interface{}

// InstanceStatusSubscription defines the event when
// the provided instance $instance_name
// changes to the provided status $status in the database
var InstanceStatusSubscription struct {
	CloudInstanceInfo struct {
		InstanceName graphql.String `graphql:"instance_name"`
		Status       graphql.String `graphql:"status"`
	} `graphql:"cloud_instance_info(where: {instance_name: {_eq: $instance_name}, _and: {status: {_eq: $status}}})"`
}

// MandelboxInfoSubscription defines the event when
// a mandelbox is assigned to the provided $instance_name
var MandelboxInfoSubscription struct {
	CloudMandelboxInfo struct {
		InstanceName graphql.String `graphql:"instance_name"`
		MandelboxID  graphql.String `graphql:"mandelbox_id"`
		SessionID    graphql.String `graphql:"session_id"`
		UserID       graphql.String `graphql:"user_id"`
		Status       graphql.String `graphql:"status"`
	} `graphql:"cloud_mandelbox_info(where: {instance_name: {_eq: $instance_name}, _and: {status: {_eq: $status}}})"`
}

// MandelboxInfoSubscription defines the event when
// a mandelbox's status changes.
var MandelboxStatusSubscription struct {
	CloudMandelboxInfo struct {
		InstanceName graphql.String `graphql:"instance_name"`
		MandelboxID  graphql.String `graphql:"mandelbox_id"`
		SessionID    graphql.String `graphql:"session_id"`
		UserID       graphql.String `graphql:"user_id"`
		Status       graphql.String `graphql:"status"`
	} `graphql:"cloud_mandelbox_info(where: {status: {_eq: $status}}})"`
}
