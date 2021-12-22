package scaling_algorithms

import (
	"context"

	"github.com/hasura/go-graphql-client"
	"github.com/whisthq/whist/backend/services/scaling-service/hosts"
	"github.com/whisthq/whist/backend/services/subscriptions"
	"github.com/whisthq/whist/backend/services/utils"
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

func (s *DefaultScalingAlgorithm) ScaleDownIfNecessary(scalingCtx context.Context, host hosts.HostHandler, event ScalingEvent) error {
	// check database for active instances without mandelboxes
	freeInstancesQuery := &subscriptions.QueryFreeInstances
	queryParams := map[string]interface{}{
		"num_mandelboxes": graphql.Int(0),
	}

	s.GraphQLClient.Query(scalingCtx, freeInstancesQuery, queryParams)

	// Verify if there are free instances that can be scaled down
	freeInstances := len(freeInstancesQuery.CloudInstanceInfo)
	if freeInstances == 0 {
		logger.Info("There are no available instances to scale down.")
		return nil
	}

	if freeInstances == DEFAULT_INSTANCE_BUFFER {
		logger.Info("Not scaling down instances as buffer is complete.")
		return nil
	}

	if freeInstances < DEFAULT_INSTANCE_BUFFER {
		logger.Warningf("Available instances are less than desired buffer, scaling up to match %v", DEFAULT_INSTANCE_BUFFER)
		// TODO: ScaleUpIfNecessary()
	}

	logger.Info("Scaling down %v free instances.", freeInstances)

	drainMutation := &subscriptions.UpdateInstanceStatus
	mutationParams := map[string]interface{}{
		"status": graphql.String("DRAINING"),
	}

	for _, instance := range freeInstancesQuery.CloudInstanceInfo {
		err := s.GraphQLClient.Mutate(scalingCtx, drainMutation, mutationParams)
		if err != nil {
			logger.Errorf("Failed to mark instance %v as draining.", instance)
		}

		logger.Info("Marked instance %v as draining on database.", instance)
		// TODO: verify scale down for each drained instance
	}

	return nil
}

func (s *DefaultScalingAlgorithm) ScaleUpIfNecessary(scalingCtx context.Context, host hosts.HostHandler, event ScalingEvent) error {
	return nil
}

func (s *DefaultScalingAlgorithm) UpgradeImage(scalingCtx context.Context, host hosts.HostHandler, event ScalingEvent, image subscriptions.Image) error {
	// create instance buffer with new image

	// wait for buffer to be ready

	// drain instances with old image

	// swapover active image on database
	return nil
}
