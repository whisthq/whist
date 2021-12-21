package scaling_algorithms

import (
	"context"

	"github.com/hasura/go-graphql-client"
	"github.com/whisthq/whist/core-go/subscriptions"
	"github.com/whisthq/whist/core-go/utils"
	logger "github.com/whisthq/whist/core-go/whistlogger"
	"github.com/whisthq/whist/scaling-service/hosts"
)

func (s *DefaultScalingAlgorithm) VerifyInstanceScaleDown(scalingCtx context.Context, host hosts.HostHandler, event ScalingEvent, instance subscriptions.Instance) error {
	logger.Infof("Verifying instance scale down for event: %v", event)

	// First, verify if the draining instance has mandelboxes running
	mandelboxesRunningQuery := &subscriptions.QueryMandelboxesByInstanceName
	queryParams := map[string]interface{}{
		"instance_name": graphql.String(instance.Name),
		"status":        graphql.String("RUNNING"),
	}

	err := s.GraphQLClient.Query(scalingCtx, mandelboxesRunningQuery, queryParams)
	if err != nil {
		return utils.MakeError("failed to query database for running mandelboxes with params: %v. Error: %v", queryParams, err)
	}

	// If instance has active mandelboxes, leave it alone
	runningMandelboxes := len(mandelboxesRunningQuery.CloudMandelboxInfo)
	if runningMandelboxes > 0 {
		logger.Infof("Instance has %v running mandelboxes. Not marking as draining.", runningMandelboxes)
		return nil
	}

	// If not, wait until the instance is terminated from the cloud provider
	// err = host.WaitForInstanceTermination(scalingCtx, instance)
	// if err != nil {
	// 	// Err is already wrapped here
	// 	return err
	// }

	// Once its terminated, verify that it was removed from the database
	instanceExistsQuery := &subscriptions.QueryInstanceByName
	queryParams = map[string]interface{}{
		"instance_name": graphql.String(instance.Name),
	}

	err = s.GraphQLClient.Query(scalingCtx, instanceExistsQuery, queryParams)
	if err != nil {
		return utils.MakeError("failed to query database for instance with params: %v. Error: %v", queryParams, err)
	}

	// Verify that instance removed itself from the database
	instanceResult := instanceExistsQuery.CloudInstanceInfo
	if len(instanceResult) == 0 {
		logger.Info("Instance %v was successfully removed from database.", instance.Name)
		return nil
	}

	// If instance still exists on the database, forcefully delete as it no longer exists on cloud provider
	logger.Info("Forcefully removing instance %v from database as it no longer exists on cloud provider.", instance.Name)

	instanceDeleteMutation := &subscriptions.DeleteInstanceByName
	deleteParams := map[string]interface{}{
		"instance_name": graphql.String(instance.Name),
	}

	err = s.GraphQLClient.Mutate(scalingCtx, instanceDeleteMutation, deleteParams)
	if err != nil {
		return utils.MakeError("failed to delete instance from database with params: %v. Error: %v", queryParams, err)
	}

	logger.Infof("Deleted %v rows from database.", instanceDeleteMutation.MutationResponse.AffectedRows)

	return nil
}
