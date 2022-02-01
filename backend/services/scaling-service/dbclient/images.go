package dbclient

import (
	"context"

	"github.com/hasura/go-graphql-client"
	"github.com/whisthq/whist/backend/services/subscriptions"
)

// QueryImage queries the database for an image that matches the given id.
func QueryImage(scalingCtx context.Context, graphQLClient *subscriptions.GraphQLClient, provider string, region string) (subscriptions.WhistImages, error) {
	latestImageQuery := subscriptions.QueryLatestImage
	queryParams := map[string]interface{}{
		"provider": graphql.String(provider),
		"region":   graphql.String(region),
	}

	err := graphQLClient.Query(scalingCtx, &latestImageQuery, queryParams)
	return latestImageQuery.WhistImages, err
}

// UpdateImage updates the received fields on the database.
func UpdateImage(scalingCtx context.Context, graphQLClient *subscriptions.GraphQLClient, image subscriptions.Image) (int, error) {
	updateMutation := subscriptions.UpdateImage

	mutationParams := map[string]interface{}{
		"provider":   graphql.String(image.Provider),
		"region":     graphql.String(image.Region),
		"image_id":   graphql.String(image.ImageID),
		"client_sha": graphql.String(image.ClientSHA),
		"updated_at": timestamptz{image.UpdatedAt},
	}

	err := graphQLClient.Mutate(scalingCtx, &updateMutation, mutationParams)
	return int(updateMutation.MutationResponse.AffectedRows), err
}
