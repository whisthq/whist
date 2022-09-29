package subscriptions // import "github.com/whisthq/whist/backend/services/subscriptions"

import (
	"time"

	"github.com/google/uuid"
	graphql "github.com/hasura/go-graphql-client"
	"github.com/whisthq/whist/backend/services/types"
)

// Types used for GraphQL queries/subscriptions

// GraphQLQuery is a custom empty interface to represent the graphql queries described in the
// `queries.go` file. An advantage is that these queries can be used both as subscriptions and normal queries.
type GraphQLQuery interface{}

// WhistInstance is the mapping of the `whist.hosts` table. This type interacts directly
// with the GraphQL client, and uses custom GraphQL types to marshal/unmarshal. Only use for GraphQL
// operations. For operations that do not interact with the client, use the `Instance` type instead.
type WhistInstance struct {
	ID                graphql.String   `graphql:"id"`
	Provider          graphql.String   `graphql:"provider"`
	Region            graphql.String   `graphql:"region"`
	ImageID           graphql.String   `graphql:"image_id"`
	ClientSHA         graphql.String   `graphql:"client_sha"`
	IPAddress         string           `graphql:"ip_addr"`
	Type              graphql.String   `graphql:"instance_type"`
	RemainingCapacity graphql.Int      `graphql:"remaining_capacity"`
	Status            graphql.String   `graphql:"status"`
	CreatedAt         time.Time        `graphql:"created_at"`
	UpdatedAt         time.Time        `graphql:"updated_at"`
	Mandelboxes       []WhistMandelbox `graphql:"mandelboxes"`
}

// WhistMandelbox is the mapping of the `whist.mandelboxes` table. This type interacts directly
// with the GraphQL client, and uses custom GraphQL types to marshal/unmarshal. Only use for GraphQL
// operations. For operations that do not interact with the client, use the `Mandelbox` type instead.
type WhistMandelbox struct {
	ID         graphql.String `graphql:"id"`
	App        graphql.String `graphql:"app"`
	InstanceID graphql.String `graphql:"instance_id"`
	UserID     graphql.String `graphql:"user_id"`
	SessionID  graphql.String `graphql:"session_id"`
	Status     graphql.String `graphql:"status"`
	CreatedAt  time.Time      `graphql:"created_at"`
	UpdatedAt  time.Time      `graphql:"updated_at"`
}

// WhistImage is the mapping of the `whist.images` table. This type interacts directly
// with the GraphQL client, and uses custom GraphQL types to marshal/unmarshal. Only use for GraphQL
// operations. For operations that do not interact with the client, use the `Image` type instead.
type WhistImage struct {
	Provider  graphql.String `graphql:"provider"`
	Region    graphql.String `graphql:"region"`
	ImageID   graphql.String `graphql:"image_id"`
	ClientSHA graphql.String `graphql:"client_sha"`
	UpdatedAt time.Time      `graphql:"updated_at"`
}

// WhistFrontendVersion is the mapping of the `desktop_app_version` table on the config database.
// This type interacts directly with the GraphQL client, and uses custom GraphQL types to marshal/unmarshal.
// Only use for GraphQL operations. For operations that do not interact with the client, use the
// `ClientAppVersion` type instead.
type WhistFrontendVersion struct {
	ID                graphql.Int    `graphql:"id"`
	Major             graphql.Int    `graphql:"major"`
	Minor             graphql.Int    `graphql:"minor"`
	Micro             graphql.Int    `graphql:"micro"`
	DevRC             graphql.Int    `graphql:"dev_rc"`
	StagingRC         graphql.Int    `graphql:"staging_rc"`
	DevCommitHash     graphql.String `graphql:"dev_commit_hash"`
	StagingCommitHash graphql.String `graphql:"staging_commit_hash"`
	ProdCommitHash    graphql.String `graphql:"prod_commit_hash"`
}

// WhistClientAppVersions is the mapping of the `dev`, `staging` and `prod` tables on the config database.
// This type interacts directly with the GraphQL client, and uses custom GraphQL types to marshal/unmarshal.
// Only use for GraphQL operations.
type WhistConfigs []struct {
	Key   graphql.String `graphql:"key"`
	Value graphql.String `graphql:"value"`
}

// HasuraParams contains the Heroku URL and Admin AccessKey to pass
// to the client during initialization.
type HasuraParams struct {
	URL       string
	AccessKey string
}

// Types used for development that don't interact with database

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
	RemainingCapacity int       `json:"remaining_capacity"` // Number of mandelboxes that can be allocated
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
	UpdatedAt  time.Time         `json:"updated_at"`  // Timestamp when the mandelbox was last updated
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

// FrontendVersion is a custom type to represent the frontend version. This type is
// meant to be used across the codebase for any operation that does not interact with
// the GraphQL client. For operations that interact with it, use the `WhistClientAppVersions` type.
type FrontendVersion struct {
	ID                int    `json:"id"`
	Major             int    `json:"major"`
	Minor             int    `json:"minor"`
	Micro             int    `json:"micro"`
	DevRC             int    `json:"dev_rc"`
	StagingRC         int    `json:"staging_rc"`
	DevCommitHash     string `json:"dev_commit_hash"`
	StagingCommitHash string `json:"staging_commit_hash"`
	ProdCommitHash    string `json:"prod_commit_hash"`
}

// Handlerfn is used to send subscription handlers to the Subscribe function.
type Handlerfn func(SubscriptionEvent, map[string]interface{}) bool

// HasuraSubscription holds the graphql query and parameters to start the subscription.
// It represents a generic subscription.
type HasuraSubscription struct {
	Query     GraphQLQuery
	Variables map[string]interface{}
	Result    SubscriptionEvent
	Handler   Handlerfn
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
// `whist.mandelbox` database table. This struct is
// meant to be used by any event that operates on the
// whist_mandelbox database table.
type MandelboxEvent struct {
	Mandelboxes []Mandelbox `json:"whist_mandelboxes"`
}

// ImageEvent represents an occurred event on the
// `whist.image` database table. This struct is
// meant to be used by any event that operates on the
// whist_images database table.
type ImageEvent struct {
	Images []Image `json:"whist_images"`
}

// FrontendVersionEvent represents an occurred event on the
// `dekstop_app_version` config database table. This struct is
// meant to be used by any event that operates on the
// desktop_app_version database table.
type FrontendVersionEvent struct {
	FrontendVersions []FrontendVersion `json:"desktop_app_version"`
}

// Helper function to convert between types

// ToInstances converts a result obtained from GraphQL of type `WhistInstance`
// to a slice of type `Instance` for convenience.
func ToInstances(dbInstances []WhistInstance) []Instance {
	var instances []Instance
	for _, instance := range dbInstances {
		instances = append(instances, Instance{
			ID:                string(instance.ID),
			Provider:          string(instance.Provider),
			Region:            string(instance.Region),
			ImageID:           string(instance.ImageID),
			ClientSHA:         string(instance.ClientSHA),
			IPAddress:         instance.IPAddress,
			Type:              string(instance.Type),
			RemainingCapacity: int(instance.RemainingCapacity),
			Status:            string(instance.Status),
			CreatedAt:         instance.CreatedAt,
			UpdatedAt:         instance.UpdatedAt,
		})
	}

	return instances
}

// ToInstances converts a result obtained from GraphQL of type `WhistMandelbox`
// to a slice of type `Mandelbox` for convenience.
func ToMandelboxes(dbMandelboxes []WhistMandelbox) []Mandelbox {
	var mandelboxes []Mandelbox
	for _, mandelbox := range dbMandelboxes {
		mandelboxes = append(mandelboxes, Mandelbox{
			ID:         types.MandelboxID(uuid.MustParse(string(mandelbox.ID))),
			App:        string(mandelbox.App),
			InstanceID: string(mandelbox.InstanceID),
			UserID:     types.UserID(string(mandelbox.UserID)),
			SessionID:  string(mandelbox.SessionID),
			Status:     string(mandelbox.Status),
			CreatedAt:  mandelbox.CreatedAt,
			UpdatedAt:  mandelbox.UpdatedAt,
		})
	}

	return mandelboxes
}

// ToImages converts a result obtained from GraphQL of type `WhistImage`
// to a slice of type `Image` for convenience.
func ToImages(dbImages []WhistImage) []Image {
	var images []Image
	for _, image := range dbImages {
		images = append(images, Image{
			Provider:  string(image.Provider),
			Region:    string(image.Region),
			ImageID:   string(image.ImageID),
			ClientSHA: string(image.ClientSHA),
			UpdatedAt: image.UpdatedAt,
		})
	}

	return images
}

// ToFrontendVersion converts a result obtained from GraphQL of type `WhistFrontendVersion`
// to a type `FrontendVersion` for convenience.
func ToFrontendVersion(dbVersion WhistFrontendVersion) FrontendVersion {
	return FrontendVersion{
		ID:                int(dbVersion.ID),
		Major:             int(dbVersion.Major),
		Minor:             int(dbVersion.Minor),
		Micro:             int(dbVersion.Micro),
		DevRC:             int(dbVersion.DevRC),
		StagingRC:         int(dbVersion.StagingRC),
		DevCommitHash:     string(dbVersion.DevCommitHash),
		StagingCommitHash: string(dbVersion.StagingCommitHash),
		ProdCommitHash:    string(dbVersion.ProdCommitHash),
	}
}
