/*
	This file contains the types used to process
	subscription results.
*/
package subscriptions // import "github.com/fractal/fractal/host-service/subscriptions"

// Instance represents instance_info in the database.
// These fields are defined in queries.go
type Instance struct {
	Instance_name string
	Status        string
}

// SubscriptionStatusResult is a struct used to hold
// the subscription results. Hasura always returns an array
// as a result.
type SubscriptionStatusResult struct {
	Hardware_instance_info []Instance
}
