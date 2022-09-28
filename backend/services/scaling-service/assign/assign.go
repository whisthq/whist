package assign

import (
	"context"

	"github.com/whisthq/whist/backend/services/host-service/dbdriver"
	"github.com/whisthq/whist/backend/services/scaling-service/config"
	"github.com/whisthq/whist/backend/services/scaling-service/dbclient"
	"github.com/whisthq/whist/backend/services/subscriptions"
	"github.com/whisthq/whist/backend/services/utils"
	logger "github.com/whisthq/whist/backend/services/whistlogger"
)

// CheckForExistingMandelbox queries the database to compute the current number of active mandelboxes associated to a single user.
// This function return a boolean indicating whether the user should be allocated a new CheckForExistingMandelbox or the request
// should be rejected if the mandelbox limit is exceeded. The caller is responsible for handling the response to the assign request.
func CheckForExistingMandelbox(ctx context.Context, db dbclient.WhistDBClient, graphQLClient subscriptions.WhistGraphQLClient, userID string) (bool, error) {
	if userID == "" {
		return false, utils.MakeError("user ID is empty, not assigning mandelboxes")
	}

	mandelboxResult, err := db.QueryUserMandelboxes(ctx, graphQLClient, userID)
	if err != nil {
		return false, err
	}

	logger.Infof("User %s has %d mandelboxes active", userID, len(mandelboxResult))

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

	if activeOrConnectingMandelboxes >= config.GetMandelboxLimitPerUser() {
		return false, nil
	}

	return true, nil
}
