package scaling_algorithms

import (
	"time"

	"github.com/hasura/go-graphql-client"
)

// The following types are not idiomatic go, but are necessary so that Hasura
// can properly recognize mutation inputs and enum types. Since they nanme must
// be lowercase, they have to exist in the `scaling_algorithms` package.

// whist_instances_insert_input is a type for used for the GraphQL mutations.
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

// timestamptz is a type used for updating graphql fields of type Time.
type timestamptz struct{ time.Time }
