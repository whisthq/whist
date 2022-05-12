package dbclient

import (
	"context"

	"github.com/hasura/go-graphql-client"
	"github.com/whisthq/whist/backend/services/subscriptions"
)

// QueryInstance queries for a mandelbox on the given instance with the given status.
func (client *DBClient) QueryMandelbox(scalingCtx context.Context, graphQLClient subscriptions.WhistGraphQLClient, instanceID string, status string) (subscriptions.WhistMandelboxes, error) {
	mandelboxQuery := subscriptions.QueryMandelboxesByInstanceId
	queryParams := map[string]interface{}{
		"instance_id": graphql.String(instanceID),
		"status":      graphql.String(status),
	}
	err := graphQLClient.Query(scalingCtx, &mandelboxQuery, queryParams)

	return mandelboxQuery.WhistMandelboxes, err
}

// InsertMandelboxes adds the received mandelboxes to the database.
func (client *DBClient) UpdateMandelbox(scalingCtx context.Context, graphQLClient subscriptions.WhistGraphQLClient, updateParams subscriptions.Mandelbox) (int, error) {
	updateMutation := subscriptions.UpdateMandelbox

	// Due to some quirks with the Hasura client, we have to convert the
	// mandelbox to `whist_mandelboxes_set_input`.
	mandelboxForDb := whist_mandelboxes_set_input{
		ID:         graphql.String(updateParams.ID.String()),
		App:        graphql.String(updateParams.App),
		InstanceID: graphql.String(updateParams.InstanceID),
		UserID:     graphql.String(updateParams.UserID),
		SessionID:  graphql.String(updateParams.SessionID),
		Status:     graphql.String(updateParams.Status),
		CreatedAt:  updateParams.CreatedAt,
		UpdatedAt:  updateParams.UpdatedAt,
	}

	mutationParams := map[string]interface{}{
		"id":      graphql.String(updateParams.ID.String()),
		"changes": mandelboxForDb,
	}
	err := graphQLClient.Mutate(scalingCtx, &updateMutation, mutationParams)
	return int(updateMutation.MutationResponse.AffectedRows), err
}
