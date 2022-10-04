package dbclient

import (
	"fmt"
	"time"

	"github.com/hasura/go-graphql-client"
)

// The timestamptz2 type is an alias for the Time type from the standard library
// time package. Graphql query variables that are timestamps should be instances
// of the timestamptz2 type. For example:
//
//	var c graphql.Client
//	var q struct { ... }
//
//	v := map[string]interface{}{
//	    "now": timestamptz2(time.Now()),
//	}
//
//	_ := client.Query(context.TODO(), &q, v)
//
// This type serves two purposes:
//
//  1. It allows us to provide a custom implementation of the Marshaler
//     interface from the json package. The graphql client encodes query
//     variables as JSON before it passes them to the graphql server. Specifying
//     a custom implementation of the Marshaler interface for the timestamptz2
//     type allows us to ensure that timestamp variables are serialized
//     correctly.
//  2. It specifies the name of the variable's graphql type. In particular,
//     timestamptz. Had we not implemented the GraphQLType interface from the
//     github.com/hasura/go-graphql-client package, the graphql type name would
//     have matched the Go type name, i.e. timestamptz2.
type timestamptz2 time.Time

// MarshalJSON implements the Marshaler interface from the standard library
// encoding/json package. It marshals a timestamptz into a JSON string
// containing a timestamp formatted according to the ISO 8601 standard.
func (t timestamptz2) MarshalJSON() ([]byte, error) {
	return []byte(fmt.Sprintf(`"%s"`, time.Time(t).Format(time.RFC3339))), nil
}

func (t timestamptz2) GetGraphQLType() string {
	return "timestamptz"
}

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

// whist_mandelboxes_insert_input is a type for used for the GraphQL mutations
// that insert rows to the `whist.mandelboxes` database table.
type whist_mandelboxes_insert_input struct {
	ID         graphql.String `json:"id"`
	App        graphql.String `json:"app"`
	InstanceID graphql.String `json:"instance_id"`
	UserID     graphql.String `json:"user_id"`
	SessionID  graphql.String `json:"session_id"`
	Status     graphql.String `json:"status"`
	CreatedAt  time.Time      `json:"created_at"`
	UpdatedAt  time.Time      `json:"updated_at"`
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
