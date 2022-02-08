package dbclient

import (
	"context"

	"github.com/hasura/go-graphql-client"
	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/subscriptions"
)

// QueryInstance queries the database for an instance with the received id.
func QueryInstance(scalingCtx context.Context, graphQLClient *subscriptions.GraphQLClient, instanceID string) (subscriptions.WhistInstances, error) {
	instancesQuery := subscriptions.QueryInstanceById
	queryParams := map[string]interface{}{
		"id": graphql.String(instanceID),
	}
	err := graphQLClient.Query(scalingCtx, &instancesQuery, queryParams)

	return instancesQuery.WhistInstances, err
}

// QueryInstancesByStatusOnRegion queries the database for all instances with the given status on the given region.
func QueryInstancesByStatusOnRegion(scalingCtx context.Context, graphQLClient *subscriptions.GraphQLClient, status string, region string) (subscriptions.WhistInstances, error) {
	instancesQuery := subscriptions.QueryInstancesByStatusOnRegion
	queryParams := map[string]interface{}{
		"status": graphql.String(status),
		"region": graphql.String(region),
	}
	err := graphQLClient.Query(scalingCtx, &instancesQuery, queryParams)

	return instancesQuery.WhistInstances, err
}

// QueryInstancesByImage queries the database for instances that match the given image id.
func QueryInstancesByImage(scalingCtx context.Context, graphQLClient *subscriptions.GraphQLClient, imageID string) (subscriptions.WhistInstances, error) {
	instancesQuery := subscriptions.QueryInstancesByImageID
	queryParams := map[string]interface{}{
		"image_id": graphql.String(imageID),
	}

	err := graphQLClient.Query(scalingCtx, &instancesQuery, queryParams)

	return instancesQuery.WhistInstances, err
}

// InsertInstances adds the received instances to the database.
func InsertInstances(scalingCtx context.Context, graphQLClient *subscriptions.GraphQLClient, insertParams []subscriptions.Instance) (int, error) {
	insertMutation := subscriptions.InsertInstances

	var instancesForDb []whist_instances_insert_input

	// Due to some quirks with the Hasura client, we have to convert the
	// slice of instances to a slice of `whist_instances_insert_input`.
	for _, instance := range insertParams {
		instancesForDb = append(instancesForDb, whist_instances_insert_input{
			ID:                graphql.String(instance.ID),
			Provider:          graphql.String(instance.Provider),
			Region:            graphql.String(instance.Region),
			ImageID:           graphql.String(instance.ImageID),
			ClientSHA:         graphql.String(metadata.GetGitCommit()),
			IPAddress:         instance.IPAddress,
			Type:              graphql.String(instance.Type),
			RemainingCapacity: graphql.Int(instance.RemainingCapacity),
			Status:            graphql.String(instance.Status),
			CreatedAt:         instance.CreatedAt,
			UpdatedAt:         instance.UpdatedAt,
		})
	}

	mutationParams := map[string]interface{}{
		"objects": instancesForDb,
	}
	err := graphQLClient.Mutate(scalingCtx, &insertMutation, mutationParams)
	return int(insertMutation.MutationResponse.AffectedRows), err
}

// UpdateInstance updates the received fields on the database.
func UpdateInstance(scalingCtx context.Context, graphQLClient *subscriptions.GraphQLClient, updateParams map[string]interface{}) (int, error) {
	updateMutation := subscriptions.UpdateInstanceStatus
	err := graphQLClient.Mutate(scalingCtx, &updateMutation, updateParams)
	return int(updateMutation.MutationResponse.AffectedRows), err
}

// DeleteInstance removes an instance with the given id from the database.
func DeleteInstance(scalingCtx context.Context, graphQLClient *subscriptions.GraphQLClient, instanceID string) (int, error) {
	deleteMutation := &subscriptions.DeleteInstanceById
	deleteParams := map[string]interface{}{
		"id": graphql.String(instanceID),
	}
	err := graphQLClient.Mutate(scalingCtx, deleteMutation, deleteParams)

	return int(deleteMutation.MutationResponse.AffectedRows), err
}
