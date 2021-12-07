package subscriptions // import "github.com/fractal/whist/host-service/subscriptions"

import mandelboxtypes "github.com/fractal/whist/host-service/mandelbox/types"

// HasuraParams contains the Heroku URL and Admin AccessKey to pass
// to the client during initialization.
type HasuraParams struct {
	URL       string
	AccessKey string
}

// Instance represents a row from the "instance_info" table
// in the database. These fields are defined in queries.go
type Instance struct {
	InstanceName string `json:"instance_name"`
	Status       string `json:"status"`
}

// Mandelbox represents a row from the "mandelbox_info" table
// in the database. These fields are defined in queries.go
type Mandelbox struct {
	InstanceName string                     `json:"instance_name"`
	ID           mandelboxtypes.MandelboxID `json:"mandelbox_id"`
	SessionID    string                     `json:"session_id"`
	UserID       mandelboxtypes.UserID      `json:"user_id"`
}

// InstanceInfoResult is a struct used to hold results for any
// subscription to the "instance_info" table. The CloudInstanceInfo
// interface represents the table `instance_info` from the `cloud` schema on the database.
type InstanceInfoResult struct {
	CloudInstanceInfo interface{} `json:"cloud_instance_info"`
}

// MandelboxInfoResult is a struct used to hold results for any
// subscription to the "mandelbox_info" table. The CloudMandelboxInfo
// interface represents the table `mandelbox_info` from the `cloud` schema on the database.
type MandelboxInfoResult struct {
	CloudMandelboxInfo interface{} `json:"cloud_mandelbox_info"`
}

// SubscriptionEvent represents any event received from Hasura
// subscriptions. We define a custom (empty) interface to make the
// main select on `host-service.go` cleaner.
type SubscriptionEvent interface{}
