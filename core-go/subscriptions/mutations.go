package subscriptions

// DeleteInstanceStatusByName deletes an instance that matches the given instance_name.
var DeleteInstanceByName struct {
	CloudInstanceInfo `graphql:"delete_cloud_instance_info(where: {instance_name: {_eq: $instance_name}})"`
}
