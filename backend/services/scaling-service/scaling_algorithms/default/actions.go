package scaling_algorithms

import (
	"context"

	"github.com/hasura/go-graphql-client"
	"github.com/whisthq/whist/backend/services/scaling-service/hosts"
	"github.com/whisthq/whist/backend/services/subscriptions"
	"github.com/whisthq/whist/backend/services/utils"
)

func (s *DefaultScalingAlgorithm) VerifyInstanceScaleDown(scalingCtx context.Context, event ScalingEvent, instance subscriptions.Instance) error {
	logger.Infof("Verifying instance scale down for event: %v", event)

	// First, verify if the draining instance has mandelboxes running
	mandelboxesRunningQuery := &subscriptions.QueryInstanceById
	queryParams := map[string]interface{}{
		"id":     graphql.String(instance.ID),
		"status": instance_state("RUNNING"),
	}

	err := s.GraphQLClient.Query(scalingCtx, mandelboxesRunningQuery, queryParams)
	if err != nil {
		return utils.MakeError("failed to query database for running mandelboxes with params: %v. Error: %v", queryParams, err)
	}

	// If instance has active mandelboxes, leave it alone
	runningMandelboxes := len(mandelboxesRunningQuery.WhistInstances[0].Mandelboxes)
	if runningMandelboxes > 0 {
		logger.Infof("Instance has %v running mandelboxes. Not marking as draining.", runningMandelboxes)
		return nil
	}

	// If not, wait until the instance is terminated from the cloud provider
	err = s.Host.WaitForInstanceTermination(scalingCtx, []string{instance.ID})
	if err != nil {
		// Err is already wrapped here
		return err
	}

	// Once its terminated, verify that it was removed from the database
	instanceExistsQuery := &subscriptions.QueryInstanceById
	queryParams = map[string]interface{}{
		"id": graphql.String(instance.ID),
	}

	err = s.GraphQLClient.Query(scalingCtx, instanceExistsQuery, queryParams)
	if err != nil {
		return utils.MakeError("failed to query database for instance with params: %v. Error: %v", queryParams, err)
	}

	// Verify that instance removed itself from the database
	instanceResult := instanceExistsQuery.WhistInstances
	if len(instanceResult) == 0 {
		logger.Info("Instance %v was successfully removed from database.", instance.ID)
		return nil
	}

	// If instance still exists on the database, forcefully delete as it no longer exists on cloud provider
	logger.Info("Forcefully removing instance %v from database as it no longer exists on cloud provider.", instance.ID)

	instanceDeleteMutation := &subscriptions.DeleteInstanceById
	deleteParams := map[string]interface{}{
		"id": graphql.String(instance.ID),
	}

	err = s.GraphQLClient.Mutate(scalingCtx, instanceDeleteMutation, deleteParams)
	if err != nil {
		return utils.MakeError("failed to delete instance from database with params: %v. Error: %v", queryParams, err)
	}

	logger.Infof("Deleted %v rows from database.", instanceDeleteMutation.MutationResponse.AffectedRows)

	return nil
}

func (s *DefaultScalingAlgorithm) ScaleDownIfNecessary(scalingCtx context.Context, event ScalingEvent) error {

	var (
		freeInstances, lingeringInstances subscriptions.WhistInstances
		lingeringIds                      []string
	)

	// check database for active instances without mandelboxes
	activeInstancesQuery := &subscriptions.QueryInstancesByStatusOnRegion
	queryParams := map[string]interface{}{
		"status": instance_state("ACTIVE"),
		"region": graphql.String(event.Region),
	}

	err := s.GraphQLClient.Query(scalingCtx, activeInstancesQuery, queryParams)
	if err != nil {
		return utils.MakeError("failed to query database for active instances. Err: %v", err)
	}

	for _, instance := range activeInstancesQuery.WhistInstances {
		capacity := int(instanceCapacity[string(instance.Type)])
		if instance.RemainingCapacity == graphql.Int(capacity) {
			freeInstances = append(freeInstances, instance)
		}
	}

	// check database for draining instances without running mandelboxes
	lingeringInstancesQuery := &subscriptions.QueryInstancesByStatusOnRegion
	queryParams = map[string]interface{}{
		"status": instance_state("DRAINING"),
		"region": graphql.String(event.Region),
	}

	err = s.GraphQLClient.Query(scalingCtx, lingeringInstancesQuery, queryParams)
	if err != nil {
		return utils.MakeError("failed to query database for lingering instances. Err: %v", err)
	}

	for _, instance := range lingeringInstancesQuery.WhistInstances {
		if len(instance.Mandelboxes) == 0 {
			lingeringInstances = append(lingeringInstances, instance)
			lingeringIds = append(lingeringIds, string(instance.ID))
		}
	}

	// Verify if there are lingering instances that can be scaled down
	if len(lingeringInstances) > 0 {
		logger.Info("Forcefully terminating lingering instances from cloud provider.")

		_, err := s.Host.SpinDownInstances(scalingCtx, lingeringIds)
		if err != nil {
			logger.Warningf("Failed to forcefully terminate lingering instances from cloud provider. Err: %v", err)
		}

		instanceDeleteMutation := &subscriptions.DeleteInstanceById

		for _, lingeringInstance := range lingeringInstances {
			logger.Infof("Deleting instance %v from database.", lingeringInstance.ID)

			deleteParams := map[string]interface{}{
				"id": graphql.String(lingeringInstance.ID),
			}

			err = s.GraphQLClient.Mutate(scalingCtx, instanceDeleteMutation, deleteParams)
			if err != nil {
				return utils.MakeError("failed to delete instance from database with params: %v. Error: %v", queryParams, err)
			}

		}

		logger.Infof("Deleted %v rows from database.", instanceDeleteMutation.MutationResponse.AffectedRows)

	} else {
		logger.Info("There are no lingering instances to scale down on %v.", event.Region)
	}

	if len(freeInstances) < DEFAULT_INSTANCE_BUFFER {
		logger.Warningf("Available instances are less than desired buffer, scaling up to match %v", DEFAULT_INSTANCE_BUFFER)
		// wantedNumInstances := DEFAULT_INSTANCE_BUFFER - freeInstances

		// activeImageID := freeInstancesQuery.WhistInstances[0].ImageID
		// Try scale up instances to match buffer size
		// s.ScaleUpIfNecessary(wantedNumInstances, scalingCtx, event, string(activeImageID))
	}

	// Verify if there are free instances that can be scaled down
	if len(freeInstances) == 0 {
		logger.Info("There are no free instances to scale down on %v. Finishing scale down function. %v", event.Region, freeInstances)
		return nil
	}

	logger.Info("Scaling down %v free instances on %v.", len(freeInstances), event.Region)

	drainMutation := &subscriptions.UpdateInstanceStatus
	for _, instance := range activeInstancesQuery.WhistInstances {
		mutationParams := map[string]interface{}{
			"id":     graphql.String(instance.ID),
			"status": instance_state("DRAINING"),
		}

		err := s.GraphQLClient.Mutate(scalingCtx, drainMutation, mutationParams)
		if err != nil {
			logger.Errorf("Failed to mark instance %v as draining. Err: %v", instance, err)
		}

		logger.Info("Marked instance %v as draining on database.", instance.ID)
	}

	return nil
}

func (s *DefaultScalingAlgorithm) ScaleUpIfNecessary(instancesToScale int, scalingCtx context.Context, event ScalingEvent, imageID string) error {
	// Try scale up in given region
	logger.Infof("Trying to spin up %v instances on region %v with image id %v", instancesToScale, event.Region, imageID)
	instanceNum := int32(instancesToScale)

	createdInstances, err := s.Host.SpinUpInstances(scalingCtx, instanceNum, imageID)
	if err != nil {
		return utils.MakeError("Failed to spin up instances, created %v, err: %v", createdInstances, err)
	}

	// Check if we could create the desired number of instances
	if len(createdInstances) != instancesToScale {
		return utils.MakeError("Could not scale up %v instances, only scaled up %v.", instancesToScale, len(createdInstances))
	}

	// Create slice with newly created instance ids
	var (
		createdInstanceIds []string
		instancesForDb     []whist_instances_insert_input
	)

	for _, instance := range createdInstances {
		createdInstanceIds = append(createdInstanceIds, instance.ID)
		instancesForDb = append(instancesForDb, whist_instances_insert_input{
			ID:                graphql.String(instance.ID),
			Provider:          graphql.String(instance.Provider),
			Region:            graphql.String(instance.Region),
			ImageID:           graphql.String(instance.ImageID),
			ClientSHA:         graphql.String(instance.ClientSHA),
			IPAddress:         instance.IPAddress,
			RemainingCapacity: graphql.Int(instance.RemainingCapacity),
			Status:            graphql.String(instance.Status),
			CreatedAt:         instance.CreatedAt,
			UpdatedAt:         instance.UpdatedAt,
		})
		logger.Infof("Created tagged instance with ID %v", instance.ID)
	}

	// Wait for new instances to be ready before adding to db
	err = s.Host.WaitForInstanceReady(scalingCtx, createdInstanceIds)
	if err != nil {
		return utils.MakeError("error waiting for new instances to be ready. Err: %v", err)
	}

	logger.Infof("Inserting newly created instances to database.")

	// If successful, write to db
	insertMutation := &subscriptions.InsertInstances
	mutationParams := map[string]interface{}{
		"objects": instancesForDb,
	}

	err = s.GraphQLClient.Mutate(scalingCtx, insertMutation, mutationParams)
	if err != nil {
		return utils.MakeError("Failed to insert instances into database. Error: %v", err)
	}

	logger.Infof("Inserted %v rows to database.", insertMutation.MutationResponse.AffectedRows)

	return nil
}

func (s *DefaultScalingAlgorithm) UpgradeImage(scalingCtx context.Context, host hosts.HostHandler, event ScalingEvent, image subscriptions.Image) error {
	// create instance buffer with new image

	// wait for buffer to be ready

	// drain instances with old image

	// swapover active image on database
	return nil
}
