package assign

import (
	"context"

	"github.com/whisthq/whist/backend/services/host-service/dbdriver"
	"github.com/whisthq/whist/backend/services/scaling-service/config"
	"github.com/whisthq/whist/backend/services/scaling-service/dbclient"
	"github.com/whisthq/whist/backend/services/subscriptions"
)

func CheckForExistingMandelbox(ctx context.Context, db dbclient.WhistDBClient, graphQLClient subscriptions.WhistGraphQLClient, userID string) (bool, error) {
	mandelboxResult, err := db.QueryUserMandelboxes(ctx, graphQLClient, userID)
	if err != nil {
		return false, err
	}

	if len(mandelboxResult) == 0 {
		return true, nil
	}

	var activeOrConnectingMandelboxes int32

	for _, mandelbox := range mandelboxResult {
		// We consider all mandelboxes that are (or will be) running.
		if !(mandelbox.Status == string(dbdriver.MandelboxStatusDying)) {
			activeOrConnectingMandelboxes++
		}
	}

	if activeOrConnectingMandelboxes > config.GetMandelboxLimitPerUser() {
		return false, nil
	}

	return true, nil
}
