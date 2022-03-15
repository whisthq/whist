# Whist Hasura Subscriptions

This directory contains files relating to the pubsub implementation.

For the Go implementation, we use the official [Hasura client](https://github.com/hasura/go-graphql-client) which supports subscriptions out of the box.

The GraphQL queries are made on the `queries.go` file.
The functions relating to starting and stopping the Hasura client are located on the `client.go` file.
Heroku connection strings are set on the `connection.go` file
Logic for running the client and subscriptions is located on the `subscriptions.go` file.

## Adding a subscription to Hasura

For creating a new Hasura subscription follow the implementation:

```go
// InstanceStatusSubscription defines the event when
// the provided instance $instance_name
// changes to the provided status $status in the database
var InstanceStatusSubscription struct {
	CloudInstanceInfo struct {
		InstanceName graphql.String `graphql:"instance_name"`
		Status       graphql.String `graphql:"status"`
	} `graphql:"cloud_instance_info(where: {instance_name: {_eq: $instance_name}, _and: {status: {_eq: $status}}})"`
}
```

Note the struct tags have to match the name of the database columns, and the outer struct tag which represents the full query in GraphQL syntax (Use the Hasura console to help you write the query).

Once you add the subscriptions to the `queries.go` file, create your subscription handlers and then add it to the `Run` function inside `subscriptions.go` file.
