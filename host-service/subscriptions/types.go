package subscriptions // import "github.com/fractal/fractal/host-service/subscriptions"

// Instance represents instance_info in the database.
// These fields are defined in queries.go
//nolint We need to use snake case so that Hasura finds the database fields.
type Instance struct {
	InstanceName string `json:"instance_name"`
	Status       string `json:"status"`
}

// SubscriptionStatusResult is a struct used to hold
// the subscription results. Hasura always returns an array
// as a result.
type SubscriptionStatusResult struct {
	HardwareInstanceInfo []Instance `json:"hardware_instance_info"`
}
