package subscriptions // import "github.com/whisthq/whist/backend/core-go/subscriptions"

import (
	graphql "github.com/hasura/go-graphql-client"
)

// GraphQLQuery is a custom empty interface to represent the graphql queries
// described in this file. An advantage is that these queries can be used both
// as subscriptions and normal queries.
type GraphQLQuery interface{}

// CloudInstanceInfo is the mapping of the `cloud_instance_info` table.
type CloudInstanceInfo []struct {
	IP                graphql.String `graphql:"ip"`
	Location          graphql.String `graphql:"location"`
	ImageID           graphql.String `graphql:"aws_ami_id"`
	Type              graphql.String `graphql:"aws_instance_type"`
	CloudProviderID   graphql.String `graphql:"cloud_provider_id"`
	CommitHash        graphql.String `graphql:"commit_hash"`
	CreationTimeMS    graphql.Float  `graphql:"creation_time_utc_unix_ms"`
	GPUVramRemaing    graphql.Float  `graphql:"gpu_vram_remaining_kb"`
	Name              graphql.String `graphql:"instance_name"`
	LastUpdatedMS     graphql.Float  `graphql:"last_updated_utc_unix_ms"`
	MandelboxCapacity graphql.Float  `graphql:"mandelbox_capacity"`
	MemoryRemainingKB graphql.Float  `graphql:"memory_remaining_kb"`
	NanoCPUsRemaining graphql.Float  `graphql:"nanocpus_remaining"`
	Status            graphql.String `graphql:"status"`
}

// CloudMandelboxInfo is the mapping of the `cloud_mandelbox_info` table.
type CloudMandelboxInfo []struct {
	ID             graphql.String `graphql:"mandelbox_id"`
	UserID         graphql.String `graphql:"user_id"`
	InstanceName   graphql.String `graphql:"instance_name"`
	SessionID      graphql.String `graphql:"session_id"`
	CreationTimeMS graphql.Float  `graphql:"creation_time_utc_unix_ms"`
	Status         graphql.String `graphql:"status"`
}

type CloudImageInfo []struct {
	Region     graphql.String  `graphql:"region_name"`
	ID         graphql.String  `graphql:"ami_id"`
	Active     graphql.Boolean `graphql:"ami_active"`
	CommitHash graphql.String  `graphql:"client_commit_hash"`
	Protected  graphql.Boolean `graphql:"protected_from_scale_down"`
}

// QueryInstanceStatusByName returns an instance that matches the given instance_name and status.
var QueryInstanceByNameWithStatus struct {
	CloudInstanceInfo `graphql:"cloud_instance_info(where: {instance_name: {_eq: $instance_name}, _and: {status: {_eq: $status}}})"`
}

// QueryInstanceStatusByName returns an instance that matches the given instance_name and status.
var QueryInstanceByName struct {
	CloudInstanceInfo `graphql:"cloud_instance_info(where: {instance_name: {_eq: $instance_name}})"`
}

// QueryInstanceStatus returns any instance that matches the given status.
var QueryInstancesByStatus struct {
	CloudInstanceInfo `graphql:"cloud_instance_info(where: {status: {_eq: $status}})"`
}

// QueryFreeInstances returns any instance that matches the given status.
var QueryFreeInstances struct {
	CloudInstanceInfo `graphql:"cloud_instance_info(where: {num_running_mandelboxes: {_eq: $num_mandelboxes}})"`
}

// QueryMandelboxesByInstanceName returns a mandelbox associated with the given instance
// and that has the given status.
var QueryMandelboxesByInstanceName struct {
	CloudMandelboxInfo `graphql:"cloud_mandelbox_info(where: {instance_name: {_eq: $instance_name}, _and: {status: {_eq: $status}}})"`
}

// QueryMandelboxStatus returns every mandelbox that matches the given status.
var QueryMandelboxByStatus struct {
	CloudMandelboxInfo `graphql:"cloud_mandelbox_info(where: {status: {_eq: $status}})"`
}
