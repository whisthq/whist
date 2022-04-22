package dbclient

import (
	"context"
	"time"

	"github.com/hasura/go-graphql-client"
	"github.com/whisthq/whist/backend/services/subscriptions"
)

func (client *DBClient) QueryInstanceWithCapacity(scalingCtx context.Context, graphQLClient subscriptions.WhistGraphQLClient, region string) (subscriptions.WhistInstances, error) {
	// The status will always be active, because we want instances
	// that are ready to accept mandelboxes (the host service is running)
	const status = "ACTIVE"
	instancesQuery := subscriptions.QueryInstanceWithCapacity
	queryParams := map[string]interface{}{
		"region": graphql.String(region),
		"status": graphql.String(status),
	}
	err := graphQLClient.Query(scalingCtx, &instancesQuery, queryParams)

	return instancesQuery.WhistInstances, err
}

// QueryInstance queries the database for an instance with the received id.
func (client *DBClient) QueryInstance(scalingCtx context.Context, graphQLClient subscriptions.WhistGraphQLClient, instanceID string) (subscriptions.WhistInstances, error) {
	instancesQuery := subscriptions.QueryInstanceById
	queryParams := map[string]interface{}{
		"id": graphql.String(instanceID),
	}
	err := graphQLClient.Query(scalingCtx, &instancesQuery, queryParams)

	return instancesQuery.WhistInstances, err
}

// QueryInstancesByStatusOnRegion queries the database for all instances with the given status on the given region.
func (client *DBClient) QueryInstancesByStatusOnRegion(scalingCtx context.Context, graphQLClient subscriptions.WhistGraphQLClient, status string, region string) (subscriptions.WhistInstances, error) {
	instancesQuery := subscriptions.QueryInstancesByStatusOnRegion
	queryParams := map[string]interface{}{
		"status": graphql.String(status),
		"region": graphql.String(region),
	}
	err := graphQLClient.Query(scalingCtx, &instancesQuery, queryParams)

	return instancesQuery.WhistInstances, err
}

// QueryInstancesByImage queries the database for instances that match the given image id.
func (client *DBClient) QueryInstancesByImage(scalingCtx context.Context, graphQLClient subscriptions.WhistGraphQLClient, imageID string) (subscriptions.WhistInstances, error) {
	instancesQuery := subscriptions.QueryInstancesByImageID
	queryParams := map[string]interface{}{
		"image_id": graphql.String(imageID),
	}

	err := graphQLClient.Query(scalingCtx, &instancesQuery, queryParams)

	return instancesQuery.WhistInstances, err
}

// InsertInstances adds the received instances to the database.
func (client *DBClient) InsertInstances(scalingCtx context.Context, graphQLClient subscriptions.WhistGraphQLClient, insertParams []subscriptions.Instance) (int, error) {
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
			ClientSHA:         graphql.String(instance.ClientSHA),
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
func (client *DBClient) UpdateInstance(scalingCtx context.Context, graphQLClient subscriptions.WhistGraphQLClient, updateParams subscriptions.Instance) (int, error) {
	updateMutation := subscriptions.UpdateInstance

	// Due to some quirks with the Hasura client, we have to convert the
	// instance to `whist_instances_set_input`.
	instancesForDb := whist_instances_set_input{
		ID:                graphql.String(updateParams.ID),
		Provider:          graphql.String(updateParams.Provider),
		Region:            graphql.String(updateParams.Region),
		ImageID:           graphql.String(updateParams.ImageID),
		ClientSHA:         graphql.String(updateParams.ClientSHA),
		IPAddress:         updateParams.IPAddress,
		Type:              graphql.String(updateParams.Type),
		RemainingCapacity: graphql.Int(updateParams.RemainingCapacity),
		Status:            graphql.String(updateParams.Status),
		CreatedAt:         updateParams.CreatedAt,
		UpdatedAt:         time.Now(),
	}

	mutationParams := map[string]interface{}{
		"id":      graphql.String(updateParams.ID),
		"changes": instancesForDb,
	}
	err := graphQLClient.Mutate(scalingCtx, &updateMutation, mutationParams)
	return int(updateMutation.MutationResponse.AffectedRows), err
}

// DeleteInstance removes an instance with the given id from the database.
func (client *DBClient) DeleteInstance(scalingCtx context.Context, graphQLClient subscriptions.WhistGraphQLClient, instanceID string) (int, error) {
	deleteMutation := &subscriptions.DeleteInstanceById
	deleteParams := map[string]interface{}{
		"id": graphql.String(instanceID),
	}
	err := graphQLClient.Mutate(scalingCtx, deleteMutation, deleteParams)

	return int(deleteMutation.MutationResponse.AffectedRows), err
}
