package scaling_algorithms

import (
	"context"

	"github.com/hasura/go-graphql-client"
	"github.com/whisthq/whist/backend/services/scaling-service/hosts"
	"github.com/whisthq/whist/backend/services/subscriptions"
	"github.com/whisthq/whist/backend/services/utils"
	logger "github.com/whisthq/whist/backend/services/whistlogger"
)

// VerifyInstanceScaleDown is a scaling action which fires when an instace is marked as DRAINING
// on the database. Its purpose is to verify and wait until the instance is terminated from the
// cloud provider and removed from the database, if it doesn't it takes the necessary steps to
// notify and ensure the database and the cloud provider don't get out of sync.
func (s *DefaultScalingAlgorithm) VerifyInstanceScaleDown(scalingCtx context.Context, event ScalingEvent, instance subscriptions.Instance) error {
	logger.Infof("Starting verify scale down action for event: %v", event)
	defer logger.Infof("Finished verify scale down action.")

	// First, verify if the draining instance has mandelboxes running
	mandelboxesRunningQuery := subscriptions.QueryInstanceById
	queryParams := map[string]interface{}{
		"id": graphql.String(instance.ID),
	}

	for _, instance := range mandelboxesRunningQuery.WhistInstances {
		// Check if lingering instance is safe to terminate
		if len(instance.Mandelboxes) > 0 {
			logger.Infof("Not scaling down draining instance because it has active associated mandelboxes.")
			return nil
		}
	}

	// If not, wait until the host service terminates the instance.
	err := s.Host.WaitForInstanceTermination(scalingCtx, []string{instance.ID})
	if err != nil {
		// Err is already wrapped here.
		// TODO: Notify that the instance didn't terminate itself, should be investigated.
		message := `Instance %v failed to terminate correctly, either the instance doesn't exist
		on AWS or something is blocking the shut down procedure. Investigate immediately.`
		logger.Errorf(message, instance.ID)
	}

	// Once its terminated, verify that it was removed from the database
	instanceExistsQuery := subscriptions.QueryInstanceById
	queryParams = map[string]interface{}{
		"id": graphql.String(instance.ID),
	}

	err = s.GraphQLClient.Query(scalingCtx, &instanceExistsQuery, queryParams)
	if err != nil {
		return utils.MakeError("failed to query database for instance with params: %v. Error: %v", queryParams, err)
	}

	// Verify that instance removed itself from the database
	instanceResult := instanceExistsQuery.WhistInstances
	if len(instanceResult) == 0 {
		logger.Info("Instance %v was successfully removed from database.", instance.ID)
		return nil
	}

	// If instance still exists on the database, delete as it no longer exists on cloud provider
	logger.Info("Removing instance %v from database as it no longer exists on cloud provider.", instance.ID)

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

// VerifyCapacity is a scaling action which checks the database to verify if we have the desired
// capacity (instance buffer). This action is run at the end of the other actions.
func (s *DefaultScalingAlgorithm) VerifyCapacity(scalingCtx context.Context, event ScalingEvent) error {
	logger.Infof("Starting verify capacity action for event: %v", event)
	defer logger.Infof("Finished verify capacity action.")

	activeInstancesQuery := subscriptions.QueryInstancesByStatusOnRegion
	queryParams := map[string]interface{}{
		"status": instance_state("ACTIVE"),
		"region": graphql.String(event.Region),
	}

	err := s.GraphQLClient.Query(scalingCtx, &activeInstancesQuery, queryParams)
	if err != nil {
		return utils.MakeError("failed to query database for active instances. Err: %v", err)
	}

	currentlyActive := activeInstancesQuery.WhistInstances
	if len(currentlyActive) < DEFAULT_INSTANCE_BUFFER {
		logger.Infof("Current number of instances on %v is less than desired %v. Scaling up to match.", event.Region, DEFAULT_INSTANCE_BUFFER)

		// Query for the latest image id
		latestImageQuery := subscriptions.QueryLatestImage
		queryParams := map[string]interface{}{
			"provider": cloud_provider("aws"), // TODO: set different provider when doing multi-cloud.
			"region":   graphql.String(event.Region),
		}

		err := s.GraphQLClient.Query(scalingCtx, &latestImageQuery, queryParams)
		if err != nil {
			return utils.MakeError("failed to query database for active instances. Err: %v", err)
		}

		latestImageId := string(latestImageQuery.WhistImages[0].ImageID)

		// Start scale up action for desired number of instances
		wantedInstances := DEFAULT_INSTANCE_BUFFER - len(currentlyActive)
		err = s.ScaleUpIfNecessary(wantedInstances, scalingCtx, event, latestImageId)
		if err != nil {
			// err is already wrapped here
			return err
		}
	} else {
		logger.Infof("Number of active instances in %v matches desired capacity of %v.", event.Region, DEFAULT_INSTANCE_BUFFER)
	}

	return nil
}

// ScaleDownIfNecessary is a scaling action which runs every 10 minutes and scales down free and
// lingering innstances, respecting the buffer defined for each region. Free instances will be
// marked as draining, and lingering instances will be terminated and removed from the database.
func (s *DefaultScalingAlgorithm) ScaleDownIfNecessary(scalingCtx context.Context, event ScalingEvent) error {
	logger.Infof("Starting scale down action for event: %v", event)
	defer logger.Infof("Finished scale down action.")

	var (
		freeInstances, lingeringInstances subscriptions.WhistInstances
		lingeringIds                      []string
		err                               error
	)

	// check database for active instances without mandelboxes
	activeInstancesQuery := subscriptions.QueryInstancesByStatusOnRegion
	queryParams := map[string]interface{}{
		"status": instance_state("ACTIVE"),
		"region": graphql.String(event.Region),
	}

	err = s.GraphQLClient.Query(scalingCtx, &activeInstancesQuery, queryParams)
	if err != nil {
		return utils.MakeError("failed to query database for active instances. Err: %v", err)
	}

	instancesToScaleDown := len(activeInstancesQuery.WhistInstances)
	for _, instance := range activeInstancesQuery.WhistInstances {
		associatedMandelboxes := len(instance.Mandelboxes)
		wantedInstances := DEFAULT_INSTANCE_BUFFER

		if associatedMandelboxes == 0 &&
			instancesToScaleDown > wantedInstances {
			freeInstances = append(freeInstances, instance)
			instancesToScaleDown--
		}
	}

	// check database for draining instances without running mandelboxes
	lingeringInstancesQuery := subscriptions.QueryInstancesByStatusOnRegion
	queryParams = map[string]interface{}{
		"status": instance_state("DRAINING"),
		"region": graphql.String(event.Region),
	}

	err = s.GraphQLClient.Query(scalingCtx, &lingeringInstancesQuery, queryParams)
	if err != nil {
		return utils.MakeError("failed to query database for lingering instances. Err: %v", err)
	}

	for _, instance := range lingeringInstancesQuery.WhistInstances {
		// Check if lingering instance is safe to terminate
		if len(instance.Mandelboxes) == 0 {
			lingeringInstances = append(lingeringInstances, instance)
			lingeringIds = append(lingeringIds, string(instance.ID))
		} else {

			// If not, notify, could be a stuck mandelbox (check if mandelbox is > day old?)
			logger.Warningf("Instance %v has %v associated mandelboxes and is marked as Draining.", instance.ID, len(instance.Mandelboxes))
		}
	}

	// We want to verify if we have the desired capacity after scaling down.
	defer func() {
		err = s.VerifyCapacity(scalingCtx, event)
	}()

	// Verify if there are lingering instances that can be scaled down
	if len(lingeringInstances) > 0 {
		logger.Info("Forcefully terminating lingering instances from cloud provider.")

		_, err = s.Host.SpinDownInstances(scalingCtx, lingeringIds)
		if err != nil {
			logger.Warningf("Failed to forcefully terminate lingering instances from cloud provider. Err: %v", err)
		}

		// Wait for instance termination
		err = s.Host.WaitForInstanceTermination(scalingCtx, lingeringIds)
		if err != nil {
			// Err is already wrapped here
			return err
		}

		// Delete instances from database
		instanceDeleteMutation := subscriptions.DeleteInstanceById

		for _, lingeringInstance := range lingeringInstances {
			logger.Infof("Deleting instance %v from database.", lingeringInstance.ID)

			deleteParams := map[string]interface{}{
				"id": graphql.String(lingeringInstance.ID),
			}

			err = s.GraphQLClient.Mutate(scalingCtx, &instanceDeleteMutation, deleteParams)
			if err != nil {
				return utils.MakeError("failed to delete instance from database with params: %v. Error: %v", queryParams, err)
			}
		}

		logger.Infof("Deleted %v rows from database.", instanceDeleteMutation.MutationResponse.AffectedRows)

	} else {
		logger.Info("There are no lingering instances to scale down in %v.", event.Region)
	}

	// Verify if there are free instances that can be scaled down
	if len(freeInstances) == 0 {
		logger.Info("There are no free instances to scale down in %v.", event.Region)
		return nil
	}

	logger.Info("Scaling down %v free instances on %v.", len(freeInstances), event.Region)

	drainMutation := subscriptions.UpdateInstanceStatus
	for _, instance := range freeInstances {
		mutationParams := map[string]interface{}{
			"id":     graphql.String(instance.ID),
			"status": instance_state("DRAINING"),
		}

		err = s.GraphQLClient.Mutate(scalingCtx, &drainMutation, mutationParams)
		if err != nil {
			logger.Errorf("Failed to mark instance %v as draining. Err: %v", instance, err)
		}

		logger.Info("Marked instance %v as draining on database.", instance.ID)
	}

	return err
}

// ScaleUpIfNecessary is a scaling action that launched the received number of instances on
// the cloud provider and registers them on the database with the initial values.
func (s *DefaultScalingAlgorithm) ScaleUpIfNecessary(instancesToScale int, scalingCtx context.Context, event ScalingEvent, imageID string) error {
	logger.Infof("Starting scale up action for event: %v", event)
	defer logger.Infof("Finished scale up action.")

	// Try scale up in given region
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
			IPAddress:         instance.IPAddress,
			Provider:          graphql.String(instance.Provider),
			Region:            graphql.String(instance.Region),
			ImageID:           graphql.String(instance.ImageID),
			ClientSHA:         graphql.String(instance.ClientSHA),
			Type:              graphql.String(instance.Type),
			RemainingCapacity: graphql.Int(instanceCapacity[instance.Type]),
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
	insertMutation := subscriptions.InsertInstances
	mutationParams := map[string]interface{}{
		"objects": instancesForDb,
	}

	err = s.GraphQLClient.Mutate(scalingCtx, &insertMutation, mutationParams)
	if err != nil {
		return utils.MakeError("Failed to insert instances into database. Error: %v", err)
	}

	logger.Infof("Inserted %v rows to database.", insertMutation.MutationResponse.AffectedRows)

	return nil
}

// UpgradeImage is a scaling action which runs when a new version is deployed. Its responsible of
// starting a buffer of instances with the new image and scaling down instances with the previous
// image.
func (s *DefaultScalingAlgorithm) UpgradeImage(scalingCtx context.Context, host hosts.HostHandler, event ScalingEvent, image subscriptions.Image) error {
	// create instance buffer with new image

	// wait for buffer to be ready

	// drain instances with old image

	// swapover active image on database
	return nil
}
