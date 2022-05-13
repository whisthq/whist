package dbclient

import (
	"time"

	"github.com/hasura/go-graphql-client"
)

// The following types are not idiomatic go, but are necessary so that Hasura
// can properly recognize mutation inputs and enum types.

// whist_instances_insert_input is a type for used for the GraphQL mutations
// that insert to the `whist.instances` database table.
type whist_instances_insert_input struct {
	ID                graphql.String `json:"id"`
	Provider          graphql.String `json:"provider"`
	Region            graphql.String `json:"region"`
	ImageID           graphql.String `json:"image_id"`
	ClientSHA         graphql.String `json:"client_sha"`
	IPAddress         string         `json:"ip_addr"`
	Type              graphql.String `json:"instance_type"`
	RemainingCapacity graphql.Int    `json:"remaining_capacity"`
	Status            graphql.String `json:"status"`
	CreatedAt         time.Time      `json:"created_at"`
	UpdatedAt         time.Time      `json:"updated_at"`
}

// whist_instances_set_input is a type for used for the GraphQL mutations
// that update the `whist.instances` database table.
type whist_instances_set_input struct {
	ID                graphql.String `json:"id"`
	Provider          graphql.String `json:"provider"`
	Region            graphql.String `json:"region"`
	ImageID           graphql.String `json:"image_id"`
	ClientSHA         graphql.String `json:"client_sha"`
	IPAddress         string         `json:"ip_addr"`
	Type              graphql.String `json:"instance_type"`
	RemainingCapacity graphql.Int    `json:"remaining_capacity"`
	Status            graphql.String `json:"status"`
	CreatedAt         time.Time      `json:"created_at"`
	UpdatedAt         time.Time      `json:"updated_at"`
}

// whist_mandelboxes_set_input is a type for used for the GraphQL mutations
// that update a row on the `whist.mandelboxes` database table.
type whist_mandelboxes_set_input struct {
	ID         graphql.String `json:"id"`
	App        graphql.String `json:"app"`
	InstanceID graphql.String `json:"instance_id"`
	UserID     graphql.String `json:"user_id"`
	SessionID  graphql.String `json:"session_id"`
	Status     graphql.String `json:"status"`
	CreatedAt  time.Time      `json:"created_at"`
	UpdatedAt  time.Time      `json:"updated_at"`
}

// whist_images_insert_input is a type for used for the GraphQL mutations
// that insert to the `whist.images` database table.
type whist_images_insert_input struct {
	Provider  graphql.String `json:"provider"`
	Region    graphql.String `json:"region"`
	ImageID   graphql.String `json:"image_id"`
	ClientSHA graphql.String `json:"client_sha"`
	UpdatedAt time.Time      `json:"updated_at"`
}

// timestamptz is a type used for updating graphql fields of type Time.
type timestamptz struct{ time.Time }
