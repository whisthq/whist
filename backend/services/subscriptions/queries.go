package subscriptions // import "github.com/whisthq/whist/backend/services/subscriptions"

import (
	"time"

	graphql "github.com/hasura/go-graphql-client"
)

// GraphQLQuery is a custom empty interface to represent the graphql queries
// described in this file. An advantage is that these queries can be used both
// as subscriptions and normal queries.
type GraphQLQuery interface{}

// WhistInstances is the mapping of the `whist.hosts` table. This type interacts directly
// with the GraphQL client, which marshals/unmarshals using this type. Only use for GraphQL
// operations.
type WhistInstances []struct {
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
	Mandelboxes       WhistMandelboxes `graphql:"mandelboxes"`
}

// WhistMandelboxes is the mapping of the `whist.mandelboxes` table. This type interacts directly
// with the GraphQL client, which marshals/unmarshals using this type. Only use for GraphQL
// operations.
type WhistMandelboxes []struct {
	ID         graphql.String `graphql:"id"`
	App        graphql.String `graphql:"app"`
	InstanceID graphql.String `graphql:"instance_id"`
	UserID     graphql.String `graphql:"user_id"`
	SessionID  graphql.String `graphql:"session_id"`
	Status     graphql.String `graphql:"status"`
	CreatedAt  time.Time      `graphql:"created_at"`
}

// WhistImages is the mapping of the `whist.images` table. This type interacts directly
// with the GraphQL client, which marshals/unmarshals using this type. Only use for GraphQL
// operations.
type WhistImages []struct {
	Provider  graphql.String `graphql:"provider"`
	Region    graphql.String `graphql:"region"`
	ImageID   graphql.String `graphql:"image_id"`
	ClientSHA graphql.String `graphql:"client_sha"`
	UpdatedAt time.Time      `graphql:"updated_at"`
}

// QueryInstanceByIdWithStatus returns an instance that matches the given instance_name and status.
var QueryInstanceByIdWithStatus struct {
	WhistInstances `graphql:"whist_instances(where: {id: {_eq: $id}, _and: {status: {_eq: $status}}})"`
}

// QueryInstanceById returns an instance that matches the given instance_name and status.
var QueryInstanceById struct {
	WhistInstances `graphql:"whist_instances(where: {id: {_eq: $id}})"`
}

// QueryInstanceStatus returns any instance that matches the given status.
var QueryInstancesByStatus struct {
	WhistInstances `graphql:"whist_instances(where: {status: {_eq: $status}})"`
}

// QueryInstancesByImageID returns any instance with the given image id.
var QueryInstancesByImageID struct {
	WhistInstances `graphql:"whist_instances(where: {image_id: {_eq: $image_id}})"`
}

// QueryInstancesByStatusOnRegion returns any instance that matches the given status and
// is located on the given region.
var QueryInstancesByStatusOnRegion struct {
	WhistInstances `graphql:"whist_instances(where: {status: {_eq: $status} _and: {region: {_eq: $region}}})"`
}

// QueryMandelboxesById returns a mandelbox associated with the given instance
// and that has the given status.
var QueryMandelboxesByInstanceId struct {
	WhistMandelboxes `graphql:"whist_mandelboxes(where: {instance_id: {_eq: $instance_id}, _and: {status: {_eq: $status}}})"`
}

// QueryMandelboxStatus returns every mandelbox that matches the given status.
var QueryMandelboxByStatus struct {
	WhistMandelboxes `graphql:"whist_mandelboxes(where: {status: {_eq: $status}})"`
}

// QueryLatestImage returns the latest image from the database for the specified provider/region pair.
var QueryLatestImage struct {
	WhistImages `graphql:"  whist_images(where: {provider: {_eq: $provider}, _and: {region: {_eq: $region}}}, order_by: {updated_at: desc})"`
}
