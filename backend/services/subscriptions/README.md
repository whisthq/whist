# Whist Hasura Subscriptions

This directory contains files relating to the Whist pubsub implementation, which we use for our various backend services to interact between each other via our database(s). If you're not familir with a publish-subscribe (pubsub) architecture, you can read more about it [here](https://hasura.io/docs/1.0/graphql/manual/pubsub.html).

For the Go implementation, we use the official [Hasura client](https://github.com/hasura/go-graphql-client) which supports subscriptions out of the box. The code is structured as follows:
- The GraphQL queries are made on the `queries.go` file.
- The functions relating to starting and stopping the Hasura client(s) are located on the `client.go` file.
- Heroku connection strings are set on the `connection.go` file.
- Logic for running an Hasura client and subscriptions is located on the `subscriptions.go` file.

## Adding a subscription to Hasura

For creating a new Hasura subscription, follow the following implementation:

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

Note that the struct tags have to match the name of the database columns, and that of the outer struct tag which represents the full query in GraphQL syntax. You can use the Hasura console to help you write the query. 

Once you add the subscriptions to the `queries.go` file, create your subscription handlers and add it to the `Run` function inside `subscriptions.go` file. TODO: @MauAraujo please add some more detail about what happens next (i.e. where the subscrition is run, how to retrieve the data, etc.)
