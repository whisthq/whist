package subscriptions // import "github.com/fractal/fractal/host-service/subscriptions"

// HasuraParams contains the Heroku URL and Admin AccessKey to pass
// to the client during initialization.
type HasuraParams struct {
	URL       string
	AccessKey string
}

// Instance represents instance_info in the database.
// These fields are defined in queries.go
type Instance struct {
	InstanceName string `json:"instance_name"`
	Status       string `json:"status"`
}

// InstanceInfoResult is a struct used to hold
// results for any subscription to the "instance_info" table.
// Hasura always returns an array as a result.
type InstanceInfoResult struct {
	CloudInstanceInfo interface{} `json:"cloud_instance_info"`
}

// SubscriptionEvent represents any event received from Hasura
// subscriptions. It simply passes the result and any error message
// via ReturnResult.
type SubscriptionEvent interface {
	ReturnResult(result interface{}, err error)
	createResultChan()
}
