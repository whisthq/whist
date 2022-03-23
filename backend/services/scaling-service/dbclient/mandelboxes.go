package dbclient

import (
	"context"

	"github.com/hasura/go-graphql-client"
	"github.com/whisthq/whist/backend/services/subscriptions"
)

// InsertMandelboxes adds the received mandelboxes to the database.
func (client *DBClient) InsertMandelboxes(scalingCtx context.Context, graphQLClient subscriptions.WhistGraphQLClient, insertParams []subscriptions.Mandelbox) (int, error) {
	insertMutation := subscriptions.InsertMandelboxes

	var mandelboxesForDb []whist_mandelboxes_insert_input

	// Due to some quirks with the Hasura client, we have to convert the
	// slice of instances to a slice of `whist_mandelboxes_insert_input`.
	for _, mandelbox := range insertParams {
		mandelboxesForDb = append(mandelboxesForDb, whist_mandelboxes_insert_input{
			ID:         graphql.String(mandelbox.ID.String()),
			App:        graphql.String(mandelbox.App),
			InstanceID: graphql.String(mandelbox.InstanceID),
			UserID:     graphql.String(mandelbox.UserID),
			SessionID:  graphql.String(mandelbox.SessionID),
			Status:     graphql.String(mandelbox.Status),
			CreatedAt:  mandelbox.CreatedAt,
		})
	}

	mutationParams := map[string]interface{}{
		"objects": mandelboxesForDb,
	}
	err := graphQLClient.Mutate(scalingCtx, &insertMutation, mutationParams)
	return int(insertMutation.MutationResponse.AffectedRows), err
}
