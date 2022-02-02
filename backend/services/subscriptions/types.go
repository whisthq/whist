package subscriptions // import "github.com/whisthq/whist/backend/services/subscriptions"

import (
	"time"

	"github.com/whisthq/whist/backend/services/types"
)

// HasuraParams contains the Heroku URL and Admin AccessKey to pass
// to the client during initialization.
type HasuraParams struct {
	URL       string
	AccessKey string
}

// Instance is a custom type to represent an instance. This type is
// meant to be used across the codebase for any operation that does
// not interact with the GraphQL client. For operations that interact
// with it, use the `WhistInstances` type instead.
type Instance struct {
	ID                string    `json:"id"`                 // ID of the instance
	Provider          string    `json:"provider"`           // Cloud provider this instance is running on
	Region            string    `json:"region"`             // Region this instance is running on
	ImageID           string    `json:"image_id"`           // ID of the machine image used to launch this instance
	ClientSHA         string    `json:"client_sha"`         // Commit hash
	IPAddress         string    `json:"ip_addr"`            // Public IPv4 address
	Type              string    `json:"instance_type"`      // Instance type
	RemainingCapacity int64     `json:"remaining_capacity"` // Number of mandelboxes that can be allocated
	Status            string    `json:"status"`             // Current status of the instance
	CreatedAt         time.Time `json:"created_at"`         // Timestamp when the instance was creates
	UpdatedAt         time.Time `json:"updated_at"`         // Timestamp when the instance was last updated
}

// Mandelbox is a custom type to represent a mandelbox. This type is
// meant to be used across the codebase for any operation that does
// not interact with the GraphQL client. For operations that interact
// with it, use the `WhistMandelboxes` type.
type Mandelbox struct {
	ID         types.MandelboxID `json:"id"`          // UUID of the mandelbox
	App        string            `json:"app"`         // App running on the mandelbox
	InstanceID string            `json:"instance_id"` // ID of the instance in which this mandelbox is running
	UserID     types.UserID      `json:"user_id"`     // ID of the user to which the mandelbox is assigned
	SessionID  string            `json:"session_id"`  // ID of the session which is assigned to the mandelbox
	Status     string            `json:"status"`      // Current status of the mandelbox
	CreatedAt  time.Time         `json:"created_at"`  // Timestamp when the mandelbox was created
}

// Image is a custom type to represent an image. We use the cloud provider
// agnostic term `image` that refers to a machine image used to launch instances.
// This type is meant to be used across the codebase for any operation that does
// not interact with the GraphQL client. For operations that interact
// with it, use the `WhistImages` type.
type Image struct {
	Provider  string    `json:"provider"`   // Cloud provider where this image is registered
	Region    string    `json:"region"`     // Region where the image is registered
	ImageID   string    `json:"image_id"`   // ID of the image
	ClientSHA string    `json:"client_sha"` // Commit hash the image belongs to
	UpdatedAt time.Time `json:"updated_at"` // Timestamp when the image was registered
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
	Mandelboxes []Mandelbox `json:"whist_mandelboxes"`
}

// ImageEvent represents an occurred event on the
// `whist.image` database table.This struct is
// meant to be used by any event that operates on the
// whist_images database table.
type ImageEvent struct {
	Images []Image `json:"whist_images"`
}
