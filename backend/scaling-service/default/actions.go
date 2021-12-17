package scaling_algorithms

import (
	"context"

	"github.com/whisthq/whist/core-go/subscriptions"
	"github.com/whisthq/whist/core-go/utils"
	logger "github.com/whisthq/whist/core-go/whistlogger"
	"github.com/whisthq/whist/scaling-service/hosts"
)

func (s *DefaultScalingAlgorithm) VerifyInstanceScaleDown(scalingCtx context.Context, host hosts.HostHandler, event ScalingEvent, instance subscriptions.Instance) error {
	logger.Infof("Verifying instance scale down for event: %v", event)
	// First, verify if the draining instance has mandelboxes running
	logger.Infof("Querying database: %v", subscriptions.QueryMandelboxesByInstanceName)
	mandelboxesRunningQuery := subscriptions.QueryMandelboxesByInstanceName
	queryParams := map[string]interface{}{
		"instance_name": instance.Name,
	}

	err := s.GraphQLClient.Query(scalingCtx, mandelboxesRunningQuery, queryParams)
	if err != nil {
		return utils.MakeError("failed to query database for running mandelboxes with params: %v. Error: %v", queryParams, err)
	}

	// Check underlying struct received...

	// If not, wait until the instance is terminated
	err = host.WaitForInstanceTermination(scalingCtx, instance)
	if err != nil {
		// Err is already wrapped here
		return err
	}

	// Once its terminated, verify that it was removed from the database
	mandelboxExistsQuery := subscriptions.QueryMandelboxStatus
	queryParams = map[string]interface{}{
		"mandelbox_id": mandelboxExistsQuery.CloudMandelboxInfo.ID,
	}

	err = s.GraphQLClient.Query(scalingCtx, mandelboxExistsQuery, queryParams)
	if err != nil {
		return utils.MakeError("failed to query database for mandelbox with params: %v. Error: %v", queryParams, err)
	}

	// Check underlying struct received...

	// If mandelbox still exists, delete from db. Write mutation to delete mandelbox from db

	return nil
}
