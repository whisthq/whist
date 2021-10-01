package subscriptions // import "github.com/fractal/fractal/host-service/subscriptions"

import graphql "github.com/hasura/go-graphql-client"

// SubscriptionInstanceStatus defines the event when
// the provided instance $instance_name
// changes to the provided status $status in the database
var SubscriptionInstanceStatus struct {
	HardwareInstanceInfo struct {
		InstanceName graphql.String `graphql:"instance_name"`
		Status       graphql.String `graphql:"status"`
	} `graphql:"hardware_instance_info(where: {instance_name: {_eq: $instance_name}, _and: {status: {_eq: $status}}})"`
}
