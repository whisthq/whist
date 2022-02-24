package scaling_algorithms

import (
	"context"
	"time"

	"github.com/hasura/go-graphql-client"
	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/scaling-service/dbclient"
	"github.com/whisthq/whist/backend/services/scaling-service/scaling_algorithms/helpers"
	"github.com/whisthq/whist/backend/services/subscriptions"
	"github.com/whisthq/whist/backend/services/utils"
	logger "github.com/whisthq/whist/backend/services/whistlogger"
)

// VerifyInstanceScaleDown is a scaling action which fires when an instance is marked as DRAINING
// on the database. Its purpose is to verify and wait until the instance is terminated from the
// cloud provider and removed from the database, if it doesn't it takes the necessary steps to
// notify and ensure the database and the cloud provider don't get out of sync.
func (s *DefaultScalingAlgorithm) VerifyInstanceScaleDown(scalingCtx context.Context, event ScalingEvent, instance subscriptions.Instance) error {
	logger.Infof("Starting verify scale down action for event: %v", event)
	defer logger.Infof("Finished verify scale down action for event: %v.", event)

	// We want to verify if we have the desired capacity after verifying scale down.
	defer func() {
		err := s.VerifyCapacity(scalingCtx, event)
		if err != nil {
			logger.Errorf("Error verifying capacity on %v. Err: %v", event.Region, err)
		}
	}()

	// First, verify if the draining instance has mandelboxes running
	instanceResult, err := s.DBClient.QueryInstance(scalingCtx, s.GraphQLClient, instance.ID)
	if err != nil {
		return utils.MakeError("failed to query database for instance %v. Error: %v", instance.ID, err)
	}

	for _, instanceRow := range instanceResult {
		// Check if lingering instance is safe to terminate
		if len(instanceRow.Mandelboxes) > 0 {
			logger.Infof("Not scaling down draining instance because it has active associated mandelboxes.")
			return nil
		}
	}

	// If not, wait until the host service terminates the instance.
	err = s.Host.WaitForInstanceTermination(scalingCtx, maxWaitTimeTerminated, []string{instance.ID})
	if err != nil {
		// Err is already wrapped here.
		// TODO: Notify that the instance didn't terminate itself, should be investigated.
		message := `Instance %v failed to terminate correctly, either the instance doesn't exist on AWS or something is blocking the shut down procedure.`
		return utils.MakeError(message, instance.ID)
	}

	// Once its terminated, verify that it was removed from the database
	instanceResult, err = s.DBClient.QueryInstance(scalingCtx, s.GraphQLClient, instance.ID)
	if err != nil {
		return utils.MakeError("failed to query database for instance %v. Error: %v", instance.ID, err)
	}

	// Verify that instance removed itself from the database
	if len(instanceResult) == 0 {
		logger.Info("Instance %v was successfully removed from database.", instance.ID)
		return nil
	}

	// If instance still exists on the database, delete as it no longer exists on cloud provider
	logger.Info("Removing instance %v from database as it no longer exists on cloud provider.", instance.ID)

	affectedRows, err := s.DBClient.DeleteInstance(scalingCtx, s.GraphQLClient, instance.ID)
	if err != nil {
		return utils.MakeError("failed to delete instance %v from database. Error: %v", instance.ID, err)
	}

	logger.Infof("Deleted %v rows from database.", affectedRows)

	return nil
}

// VerifyCapacity is a scaling action which checks the database to verify if we have the desired
// capacity (instance buffer). This action is run at the end of the other actions.
func (s *DefaultScalingAlgorithm) VerifyCapacity(scalingCtx context.Context, event ScalingEvent) error {
	logger.Infof("Starting verify capacity action for event: %v", event)
	defer logger.Infof("Finished verify capacity action for event: %v", event)

	// Query for the latest image id
	imageResult, err := dbclient.QueryImage(scalingCtx, s.GraphQLClient, "AWS", event.Region) // TODO: set different provider when doing multi-cloud.
	if err != nil {
		return utils.MakeError("failed to query database for current image. Err: %v", err)
	}

	if len(imageResult) == 0 {
		logger.Warningf("Image not found on %v. Not performing any scaling actions.", event.Region)
		return nil
	}
	latestImageID := string(imageResult[0].ImageID)

	// This query will return all instances with the ACTIVE status
	allActive, err := dbclient.QueryInstancesByStatusOnRegion(scalingCtx, s.GraphQLClient, "ACTIVE", event.Region)
	if err != nil {
		return utils.MakeError("failed to query database for active instances. Err: %v", err)
	}

	// This query will return all instances with the PRE_CONNECTION status
	allStarting, err := dbclient.QueryInstancesByStatusOnRegion(scalingCtx, s.GraphQLClient, "PRE_CONNECTION", event.Region)
	if err != nil {
		return utils.MakeError("failed to query database for starting instances. Err: %v", err)
	}

	// The sum of active instances with space to run mandelboxes and starting instances
	expectedInstances := helpers.GetExpectedInstances(latestImageID, allActive, allStarting)

	// We consider the expected instances here (active instances + starting instances)
	// so that we don't scale up unnecessary instances.
	if expectedInstances < DEFAULT_INSTANCE_BUFFER {

		logger.Infof("Current number of instances %v is less than desired %v. Scaling up to match with image %v.", expectedInstances, DEFAULT_INSTANCE_BUFFER, latestImageID)

		// Start scale up action for desired number of instances
		wantedInstances := DEFAULT_INSTANCE_BUFFER - expectedInstances
		err = s.ScaleUpIfNecessary(wantedInstances, scalingCtx, event, latestImageID)
		if err != nil {
			// err is already wrapped here
			return err
		}
	} else {
		logger.Infof("Number of active instances in %v with image %v matches desired capacity of %v.", event.Region, latestImageID, DEFAULT_INSTANCE_BUFFER)
	}

	return nil
}

// ScaleDownIfNecessary is a scaling action which runs every 10 minutes and scales down free and
// lingering instances, respecting the buffer defined for each region. Free instances will be
// marked as draining, and lingering instances will be terminated and removed from the database.
func (s *DefaultScalingAlgorithm) ScaleDownIfNecessary(scalingCtx context.Context, event ScalingEvent) error {
	logger.Infof("Starting scale down action for event: %v", event)
	defer logger.Infof("Finished scale down action for event: %v", event)

	// We want to verify if we have the desired capacity after scaling down.
	defer func() {
		err := s.VerifyCapacity(scalingCtx, event)
		if err != nil {
			logger.Errorf("Error verifying capacity on %v. Err: %v", event.Region, err)
		}
	}()

	var (
		freeInstances, lingeringInstances subscriptions.WhistInstances
		lingeringIDs                      []string
		err                               error
	)

	// Query for the latest image id
	imageResult, err := dbclient.QueryImage(scalingCtx, s.GraphQLClient, "AWS", event.Region) // TODO: set different provider when doing multi-cloud.
	if err != nil {
		return utils.MakeError("failed to query database for current image. Err: %v", err)
	}

	if len(imageResult) == 0 {
		logger.Warningf("Image not found on %v. Not performing any scaling actions.", event.Region)
		return nil
	}
	latestImageID := string(imageResult[0].ImageID)

	// query database for all active instances (status ACTIVE) without mandelboxes
	allActive, err := dbclient.QueryInstancesByStatusOnRegion(scalingCtx, s.GraphQLClient, "ACTIVE", event.Region)
	if err != nil {
		return utils.MakeError("failed to query database for active instances. Err: %v", err)
	}

	// Active instances with space to run mandelboxes
	availableInstances := helpers.GetAvailableInstances(latestImageID, allActive)

	// Create a list of instances that can be scaled down from the active instances list.
	// For this, we have to consider the following conditions:
	// 1. Does the instance have any running mandelboxes? If so, don't scale down.
	// 2. Does the instance have the latest image, corresponding to the latest entry on the database?
	// If so, check if we have more than the desired number of instances. In case we do, add the instance
	// to the list that will be scaled down. This also checks if terminating the instance will leave us
	// with insuficient capacity, and if it does we leave the instance running.
	// 3. If the instance does not have the latest image, and is not running any mandelboxes, add to the
	// list that will be scaled down.
	for _, instance := range allActive {
		if len(instance.Mandelboxes) > 0 {
			// Don't scale down any instance that has running
			// mandelboxes, regardless of the image it uses
			logger.Infof("Not scaling down instance %v because it has %v mandelboxes running.", instance.ID, len(instance.Mandelboxes))
			continue
		}

		if instance.ImageID == graphql.String(latestImageID) {
			// Current instances
			if availableInstances > DEFAULT_INSTANCE_BUFFER {
				// Only scale down if there are more instances
				// than necessary.
				logger.Infof("Scaling down instance %v because we have more capacity of %v than desired %v.", instance.ID, availableInstances, DEFAULT_INSTANCE_BUFFER)
				freeInstances = append(freeInstances, instance)
				availableInstances--
			}
		} else {
			// Old instances
			logger.Infof("Scaling down instance %v because it has an old image id %v.", instance.ID, instance.ImageID)
			freeInstances = append(freeInstances, instance)
		}
	}

	// check database for draining instances without running mandelboxes
	drainingInstances, err := s.DBClient.QueryInstancesByStatusOnRegion(scalingCtx, s.GraphQLClient, "DRAINING", event.Region)
	if err != nil {
		return utils.MakeError("failed to query database for lingering instances. Err: %v", err)
	}

	for _, instance := range drainingInstances {
		// Check if lingering instance is free from mandelboxes
		if len(instance.Mandelboxes) == 0 {
			lingeringInstances = append(lingeringInstances, instance)
			lingeringIDs = append(lingeringIDs, string(instance.ID))
		} else {
			// If not, notify, could be a stuck mandelbox (check if mandelbox is > day old?)
			logger.Warningf("Instance %v has %v associated mandelboxes and is marked as Draining.", instance.ID, len(instance.Mandelboxes))
		}
	}

	// Verify if there are lingering instances and notify.
	if len(lingeringInstances) > 0 {
		logger.Errorf("There are %v lingering instances on %v. Investigate immediately! Their IDs are %v", len(lingeringInstances), event.Region, lingeringIDs)

	} else {
		logger.Info("There are no lingering instances in %v.", event.Region)
	}

	// Verify if there are free instances that can be scaled down
	if len(freeInstances) == 0 {
		logger.Info("There are no free instances to scale down in %v.", event.Region)
		return nil
	}

	logger.Info("Scaling down %v free instances on %v.", len(freeInstances), event.Region)

	for _, instance := range freeInstances {
		updateParams := map[string]interface{}{
			"id":     instance.ID,
			"status": graphql.String("DRAINING"),
		}

		_, err = s.DBClient.UpdateInstance(scalingCtx, s.GraphQLClient, updateParams)
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
	defer logger.Infof("Finished scale up action for event: %v", event)

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
		createdInstanceIDs []string
		instancesForDb     []subscriptions.Instance
	)

	for _, instance := range createdInstances {
		createdInstanceIDs = append(createdInstanceIDs, instance.ID)
		instancesForDb = append(instancesForDb, subscriptions.Instance{
			ID:                instance.ID,
			IPAddress:         instance.IPAddress,
			Provider:          instance.Provider,
			Region:            instance.Region,
			ImageID:           instance.ImageID,
			ClientSHA:         instance.ClientSHA,
			Type:              instance.Type,
			RemainingCapacity: int64(instanceCapacity[instance.Type]),
			Status:            instance.Status,
			CreatedAt:         instance.CreatedAt,
			UpdatedAt:         instance.UpdatedAt,
		})
		logger.Infof("Created tagged instance with ID %v", instance.ID)
	}

	// Wait for new instances to be ready before adding to db
	err = s.Host.WaitForInstanceReady(scalingCtx, maxWaitTimeReady, createdInstanceIDs)
	if err != nil {
		return utils.MakeError("error waiting for new instances to be ready. Err: %v", err)
	}

	logger.Infof("Inserting newly created instances to database.")

	// If successful, write to db
	affectedRows, err := s.DBClient.InsertInstances(scalingCtx, s.GraphQLClient, instancesForDb)
	if err != nil {
		return utils.MakeError("Failed to insert instances into database. Error: %v", err)
	}

	logger.Infof("Inserted %v rows to database.", affectedRows)

	return nil
}

// UpgradeImage is a scaling action which runs when a new version is deployed. Its responsible of
// starting a buffer of instances with the new image and scaling down instances with the previous
// image.
func (s *DefaultScalingAlgorithm) UpgradeImage(scalingCtx context.Context, event ScalingEvent, imageID interface{}) error {
	logger.Infof("Starting upgrade image action for event: %v", event)
	defer logger.Infof("Finished upgrade image action for event: %v", event)

	var oldImageID string

	// Check if we received a valid image before performing more
	// expensive operations.

	if imageID == nil {
		logger.Warningf("Received an empty image ID on %v. Not performing upgrade.", event.Region)
		return nil
	}

	newImageID := imageID.(string)

	// Query for the current image id
	imageResult, err := s.DBClient.QueryImage(scalingCtx, s.GraphQLClient, "AWS", event.Region)
	if err != nil {
		return utils.MakeError("failed to query database for current image on %v. Err: %v", event.Region, err)
	}

	if len(imageResult) == 0 {
		logger.Warningf("Image doesn't exist on %v. Creating a new entry with image %v.", event.Region, newImageID)
	} else {
		// We now consider the "current" image as the "old" image
		oldImageID = string(imageResult[0].ImageID)
	}

	// create instance buffer with new image
	logger.Infof("Creating new instance buffer for image %v", newImageID)
	bufferInstances, err := s.Host.SpinUpInstances(scalingCtx, DEFAULT_INSTANCE_BUFFER, newImageID)
	if err != nil {
		return utils.MakeError("failed to create instance buffer for image %v. Error: %v", newImageID, err)
	}

	// create slice of newly created instance ids
	var (
		bufferIDs      []string
		instancesForDb []subscriptions.Instance
	)
	for _, instance := range bufferInstances {
		bufferIDs = append(bufferIDs, instance.ID)
		instancesForDb = append(instancesForDb, subscriptions.Instance{
			ID:                instance.ID,
			IPAddress:         instance.IPAddress,
			Provider:          instance.Provider,
			Region:            instance.Region,
			ImageID:           instance.ImageID,
			ClientSHA:         instance.ClientSHA,
			Type:              instance.Type,
			RemainingCapacity: int64(instanceCapacity[instance.Type]),
			Status:            instance.Status,
			CreatedAt:         instance.CreatedAt,
			UpdatedAt:         instance.UpdatedAt,
		})
	}

	// wait for buffer to be ready.
	err = s.Host.WaitForInstanceReady(scalingCtx, maxWaitTimeReady, bufferIDs)
	if err != nil {
		return utils.MakeError("error waiting for instances to be ready. Error: %v", err)
	}

	logger.Infof("Inserting newly created instances to database.")

	// If successful, write to db
	affectedRows, err := s.DBClient.InsertInstances(scalingCtx, s.GraphQLClient, instancesForDb)
	if err != nil {
		return utils.MakeError("Failed to insert instances into database. Error: %v", err)
	}

	logger.Infof("Inserted %v rows to database.", affectedRows)

	// swapover active image on database
	logger.Infof("Updating old %v image %v to new image %v on database.", event.Region, oldImageID, newImageID)
	updateParams := subscriptions.Image{
		Provider:  "AWS",
		Region:    event.Region,
		ImageID:   newImageID,
		ClientSHA: metadata.GetGitCommit(),
		UpdatedAt: time.Now(),
	}

	if oldImageID == "" {
		_, err = s.DBClient.InsertImages(scalingCtx, s.GraphQLClient, []subscriptions.Image{updateParams})
		if err != nil {
			return utils.MakeError("Failed to insert image %v into database. Error: %v", newImageID, err)
		}
	} else {
		_, err = s.DBClient.UpdateImage(scalingCtx, s.GraphQLClient, updateParams)
		if err != nil {
			return utils.MakeError("Failed to update image %v to image %v in database. Error: %v", oldImageID, newImageID, err)
		}
	}

	return nil
}
