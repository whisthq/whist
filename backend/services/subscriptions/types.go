package subscriptions // import "github.com/whisthq/whist/backend/services/subscriptions"

import (
	"time"

	mandelboxtypes "github.com/whisthq/whist/backend/core-go/types"
)

// HasuraParams contains the Heroku URL and Admin AccessKey to pass
// to the client during initialization.
type HasuraParams struct {
	URL       string
	AccessKey string
}

// Instance represents a host from the "whist_instances" table. This type is
// meant to be used for development purposes.
type Instance struct {
	ID                string    `json:"id"`
	Provider          string    `json:"provider"`
	Region            string    `json:"region"`
	ImageID           string    `json:"image_id"`
	ClientSHA         string    `json:"client_sha"`
	IPAddress         string    `json:"ip_addr"`
	Type              string    `json:"instance_type"`
	RemainingCapacity int64     `json:"remaining_capacity"`
	Status            string    `json:"status"`
	CreatedAt         time.Time `json:"created_at"`
	UpdatedAt         time.Time `json:"updated_at"`
}

// Mandelbox represents a host from the "whist_mandelboxes" table. This type is
// meant to be used for development purposes.
type Mandelbox struct {
	ID         mandelboxtypes.MandelboxID `json:"id"`
	App        string                     `json:"app"`
	InstanceID string                     `json:"instance_id"`
	UserID     mandelboxtypes.UserID      `json:"user_id"`
	SessionID  string                     `json:"session_id"`
	Status     string                     `json:"status"`
	CreatedAt  time.Time                  `json:"created_at"`
}

// Image represents a host from the "whist_images" table. This type is
// meant to be used for development purposes.
type Image struct {
	Provider  string    `json:"provider"`
	Region    string    `json:"region"`
	ImageID   string    `json:"image_id"`
	ClientSHA string    `json:"client_sha"`
	UpdatedAt time.Time `json:"updated_at"`
}

// handlerfn is used to send subscription handlers to the Subscribe function.
type handlerfn func(SubscriptionEvent, map[string]interface{}) bool

// HasuraSubscription holds the graphql query and parameters to start the subscription.
// It represents a generic subscription.
type HasuraSubscription struct {
	Query     GraphQLQuery
	Variables map[string]interface{}
	Result    SubscriptionEvent
	Handler   handlerfn
}

// InstanceResult is a struct used to hold results for any
// subscription to the "instance_info" table. The Instances
// interface represents the table `hosts` from the `whist` schema on the database.
type InstanceResult struct {
	WhistInstances interface{} `json:"whist_instances"`
}

// MandelboxResult is a struct used to hold results for any
// subscription to the "mandelboxes" table. The WhistMandelboxes
// interface represents the table `mandelboxes` from the `whist` schema on the database.
type MandelboxResult struct {
	WhistMandelboxes interface{} `json:"whist_mandelboxes"`
}

// ImageResult is a struct used to hold results for any
// subscription to the "whist_images" table. The WhistImages
// interface represents the table `images` from the `whist` schema on the database.
type ImageResult struct {
	WhistImages interface{} `json:"whist_images"`
}

// SubscriptionEvent represents any event received from Hasura
// subscriptions. We define a custom (empty) interface to make the
// main select on `host-service.go` cleaner.
type SubscriptionEvent interface{}

// InstanceEvent represents an occurred event on the
// `whist.hosts` database table. This struct is
// meant to be used by any event that operates on the
// instance_info database table.
type InstanceEvent struct {
	Instances []Instance `json:"whist_instances"`
}

// MandelboxEvent represents an occurred event on the
// `whist.mandelbox` database table.This struct is
// meant to be used by any event that operates on the
// whist_mandelbox database table.
type MandelboxEvent struct {
	Mandelboxes []Mandelbox `json:"whist_madelboxes"`
}

// ImageEvent represents an occurred event on the
// `whist.image` database table.This struct is
// meant to be used by any event that operates on the
// whist_images database table.
type ImageEvent struct {
	Images []Image `json:"whist_images"`
}

// The following types are not idiomatic go, but are
// necessary so that Hasura can properly recognize
// the enum types on the database.
type instance_state string

type mandelbox_state string
