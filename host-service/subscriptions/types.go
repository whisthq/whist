/*
	This file contains the types used to process
	subscription results.
*/
package subscriptions // import "github.com/fractal/fractal/host-service/subscriptions"

type Instance struct {
	Instance_name string
	Status        string
}

type SubscriptionStatusResult struct {
	Hardware_instance_info []Instance
}
