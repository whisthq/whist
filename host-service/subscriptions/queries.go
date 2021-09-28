package subscriptions

import graphql "github.com/hasura/go-graphql-client"

var SubscriptionInstanceStatus struct {
	HardwareInstanceInfo struct {
		Instance_name graphql.String
		Aws_ami_id    graphql.String
		Status        graphql.String
	} `graphql:"hardware_instance_info(where: {status: {_eq: $status}})"`
	// `graphql:"hardware_instance_info(where: {instance_name: {_eq: $instance_name}, _and: {status: {_eq: $status}}})"`
}
