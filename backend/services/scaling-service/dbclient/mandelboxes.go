package dbclient

import (
	"context"

	"github.com/hasura/go-graphql-client"
	"github.com/whisthq/whist/backend/services/subscriptions"
)

// QueryMandelbox queries for a mandelbox on the given instance with the given status.
func (client *DBClient) QueryMandelbox(scalingCtx context.Context, graphQLClient subscriptions.WhistGraphQLClient, instanceID string, status string) ([]subscriptions.Mandelbox, error) {
	mandelboxQuery := subscriptions.QueryMandelboxesByInstanceId
	queryParams := map[string]interface{}{
		"instance_id": graphql.String(instanceID),
		"status":      graphql.String(status),
	}
	err := graphQLClient.Query(scalingCtx, &mandelboxQuery, queryParams)
	mandelboxResult := subscriptions.ToMandelboxes(mandelboxQuery.WhistMandelboxes)

	return mandelboxResult, err
}

// QueryUserMandelboxes find if a user has mandelboxes currently registered in the database.
func (client *DBClient) QueryUserMandelboxes(scalingCtx context.Context, graphQLClient subscriptions.WhistGraphQLClient, userID string) ([]subscriptions.Mandelbox, error) {
	mandelboxQuery := subscriptions.QueryMandelboxesByUserId
	queryParams := map[string]interface{}{
		"user_id": graphql.String(userID),
	}
	err := graphQLClient.Query(scalingCtx, &mandelboxQuery, queryParams)
	mandelboxResult := subscriptions.ToMandelboxes(mandelboxQuery.WhistMandelboxes)

	return mandelboxResult, err
}

// InsertMandelboxes adds the received mandelboxes to the database.
func (client *DBClient) InsertMandelboxes(scalingCtx context.Context, graphQLClient subscriptions.WhistGraphQLClient, insertParams []subscriptions.Mandelbox) (int, error) {
	insertMutation := subscriptions.InsertMandelboxes

	var mandelboxesForDb []whist_mandelboxes_insert_input

	for _, mandelbox := range insertParams {

		// Due to some quirks with the Hasura client, we have to convert the
		// mandelbox to `whist_mandelboxes_insert_input`.
		mandelboxesForDb = append(mandelboxesForDb, whist_mandelboxes_insert_input{
			ID:         graphql.String(mandelbox.ID.String()),
			App:        graphql.String(mandelbox.App),
			InstanceID: graphql.String(mandelbox.InstanceID),
			UserID:     graphql.String(mandelbox.UserID),
			SessionID:  graphql.String(mandelbox.SessionID),
			Status:     graphql.String(mandelbox.Status),
			CreatedAt:  mandelbox.CreatedAt,
			UpdatedAt:  mandelbox.UpdatedAt,
		})
	}

	mutationParams := map[string]interface{}{
		"objects": mandelboxesForDb,
	}
	err := graphQLClient.Mutate(scalingCtx, &insertMutation, mutationParams)
	return int(insertMutation.MutationResponse.AffectedRows), err
}

// UpdateMandelbox updates the received mandelboxes in the database.
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
