package scaling_algorithms

import (
	"context"
	"time"

	"github.com/hasura/go-graphql-client"
	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/scaling-service/scaling_algorithms/helpers"
	"github.com/whisthq/whist/backend/services/subscriptions"
	"github.com/whisthq/whist/backend/services/utils"
	logger "github.com/whisthq/whist/backend/services/whistlogger"
)

// Use a map to keep track of images that should not be scaled down.
// This should only be used in the context of a deploy to avoid downtimes.
var protectedFromScaleDown map[string]subscriptions.Image

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
	imageResult, err := s.DBClient.QueryImage(scalingCtx, s.GraphQLClient, "AWS", event.Region) // TODO: set different provider when doing multi-cloud.
	if err != nil {
		return utils.MakeError("failed to query database for current image. Err: %v", err)
	}

	if len(imageResult) == 0 {
		logger.Warningf("Image not found on %v. Not performing any scaling actions.", event.Region)
		return nil
	}
	latestImageID := string(imageResult[0].ImageID)

	// This query will return all instances with the ACTIVE status
	allActive, err := s.DBClient.QueryInstancesByStatusOnRegion(scalingCtx, s.GraphQLClient, "ACTIVE", event.Region)
	if err != nil {
		return utils.MakeError("failed to query database for active instances. Err: %v", err)
	}

	// This query will return all instances with the PRE_CONNECTION status
	allStarting, err := s.DBClient.QueryInstancesByStatusOnRegion(scalingCtx, s.GraphQLClient, "PRE_CONNECTION", event.Region)
	if err != nil {
		return utils.MakeError("failed to query database for starting instances. Err: %v", err)
	}

	mandelboxCapacity := helpers.ComputeExpectedMandelboxCapacity(latestImageID, allActive, allStarting)

	// We consider the expected mandelbox capacity (active instances + starting instances)
	// to account for warmup time and so that we don't scale up unnecesary instances.
	if mandelboxCapacity < DESIRED_FREE_MANDELBOXES {
		logger.Infof("Current mandelbox capacity of %v is less than desired %v. Scaling up %v instances to satisfy minimum desired capacity.", mandelboxCapacity, DESIRED_FREE_MANDELBOXES, DEFAULT_INSTANCE_BUFFER)
		err = s.ScaleUpIfNecessary(DEFAULT_INSTANCE_BUFFER, scalingCtx, event, latestImageID)
		if err != nil {
			// err is already wrapped here
			return err
		}
	} else {
		logger.Infof("Mandelbox capacity %v in %v is enough to satisfy minimum desired capacity of %v.", mandelboxCapacity, event.Region, DESIRED_FREE_MANDELBOXES)
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
	imageResult, err := s.DBClient.QueryImage(scalingCtx, s.GraphQLClient, "AWS", event.Region) // TODO: set different provider when doing multi-cloud.
	if err != nil {
		return utils.MakeError("failed to query database for current image. Err: %v", err)
	}

	if len(imageResult) == 0 {
		logger.Warningf("Image not found on %v. Not performing any scaling actions.", event.Region)
		return nil
	}
	latestImageID := string(imageResult[0].ImageID)

	// query database for all active instances (status ACTIVE) without mandelboxes
	allActive, err := s.DBClient.QueryInstancesByStatusOnRegion(scalingCtx, s.GraphQLClient, "ACTIVE", event.Region)
	if err != nil {
		return utils.MakeError("failed to query database for active instances. Err: %v", err)
	}

	// Active instances with space to run mandelboxes
	mandelboxCapacity := helpers.ComputeRealMandelboxCapacity(latestImageID, allActive)

	// Extra capacity is considered once we have a full instance worth of capacity
	// more than the desired free mandelboxes. TODO: Set the instance type once we
	// have support for more instance types. For now default to `g4dn.2xlarge`.
	extraCapacity := DESIRED_FREE_MANDELBOXES + (DEFAULT_INSTANCE_BUFFER * instanceCapacity["g4dn.2xlarge"])

	// Create a list of instances that can be scaled down from the active instances list.
	// For this, we have to consider the following conditions:
	// 1. Does the instance have any running mandelboxes? If so, don't scale down.
	// 2. Does the instance have the latest image, corresponding to the latest entry on the database?
	// If so, check if we have more than the desired number of mandelbox capacity. In case we do, add the instance
	// to the list that will be scaled down.
	// 3. If the instance does not have the latest image, and is not running any mandelboxes, add to the
	// list that will be scaled down.
	for _, instance := range allActive {
		if len(instance.Mandelboxes) > 0 {
			// Don't scale down any instance that has running
			// mandelboxes, regardless of the image it uses
			logger.Infof("Not scaling down instance %v because it has %v mandelboxes running.", instance.ID, len(instance.Mandelboxes))
			continue
		}

		_, protected := protectedFromScaleDown[string(instance.ImageID)]
		if protected {
			// Don't scale down instances with a protected image id. This is because the
			// image id will not be switched over until the client has updated its version
			// on the config database.
			logger.Infof("Not scaling down instance %v because it has an image id that is protected from scale down.", instance.ID)
			continue
		}

		if instance.ImageID == graphql.String(latestImageID) {
			// Current instances
			// If we have more than one instance worth of extra mandelbox capacity, scale down
			if mandelboxCapacity >= extraCapacity {
				logger.Infof("Scaling down instance %v because we have more mandelbox capacity of %v than desired %v.", instance.ID, mandelboxCapacity, extraCapacity)
				freeInstances = append(freeInstances, instance)
				mandelboxCapacity -= int(instance.RemainingCapacity)
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
		logger.Infof("Scaling down instance %v.", instance.ID)
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

	createdInstances, err := s.Host.SpinUpInstances(scalingCtx, instanceNum, maxWaitTimeReady, imageID)
	if err != nil {
		return utils.MakeError("Failed to spin up instances, created %v, err: %v", createdInstances, err)
	}

	// Check if we could create the desired number of instances
	if len(createdInstances) != instancesToScale {
		return utils.MakeError("Could not scale up %v instances, only scaled up %v.", instancesToScale, len(createdInstances))
	}

	// Create slice with newly created instance ids
	var instancesForDb []subscriptions.Instance

	for _, instance := range createdInstances {
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

	// create instance buffer with new image
	logger.Infof("Creating new instance buffer for image %v", newImageID)
	bufferInstances, err := s.Host.SpinUpInstances(scalingCtx, DEFAULT_INSTANCE_BUFFER, maxWaitTimeReady, newImageID)
	if err != nil {
		return utils.MakeError("failed to create instance buffer for image %v. Error: %v", newImageID, err)
	}

	// create slice of newly created instance ids
	var instancesForDb []subscriptions.Instance

	for _, instance := range bufferInstances {
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

	logger.Infof("Inserting newly created instances to database.")

	// If successful, write to db
	affectedRows, err := s.DBClient.InsertInstances(scalingCtx, s.GraphQLClient, instancesForDb)
	if err != nil {
		return utils.MakeError("Failed to insert instances into database. Error: %v", err)
	}

	logger.Infof("Inserted %v rows to database.", affectedRows)

	newImage := subscriptions.Image{
		Provider:  "AWS",
		Region:    event.Region,
		ImageID:   newImageID,
		ClientSHA: metadata.GetGitCommit(),
		UpdatedAt: time.Now(),
	}

	// Protect the new instance buffer from scale down. This is done to avoid any downtimes
	// during deploy, as the active image will be switched until the client app has updated
	// its version on the config database.
	protectedFromScaleDown = make(map[string]subscriptions.Image)
	protectedFromScaleDown[newImageID] = newImage

	// If the region does not have an existing image, insert the new one to the database.
	if len(imageResult) == 0 {
		logger.Warningf("Image doesn't exist on %v. Creating a new entry with image %v.", event.Region, newImageID)

		updateParams := subscriptions.Image{
			Provider:  "AWS",
			Region:    event.Region,
			ImageID:   newImageID,
			ClientSHA: metadata.GetGitCommit(),
			UpdatedAt: time.Now(),
		}

		_, err = s.DBClient.InsertImages(scalingCtx, s.GraphQLClient, []subscriptions.Image{updateParams})
		if err != nil {
			return utils.MakeError("Failed to insert image %v into database. Error: %v", newImageID, err)
		}
	}

	s.SyncChan <- true
	return nil
}

// SwapOverImages is a scaling action that will switch the current image on the given region.
// To the latest one. This is done separately to avoid having downtimes during deploys, since
// we have to wait until the client has updated its version on the config database.
func (s *DefaultScalingAlgorithm) SwapOverImages(scalingCtx context.Context, event ScalingEvent, clientVersion interface{}) error {
	// Block until the image upgrade has finished successfully
	<-s.SyncChan

	logger.Infof("Starting upgrade image swapover action for event: %v", event)
	defer logger.Infof("Finished upgrade image swapover action for event: %v", event)

	// version is the entry we receive from the config database
	version := clientVersion.(subscriptions.ClientAppVersion)

	var (
		commitHash string
		newImage   subscriptions.Image
		newImageID string
		oldImageID string
	)

	// get the commit hash of the environment we are running in
	switch metadata.GetAppEnvironmentLowercase() {
	case string(metadata.EnvDev):
		commitHash = version.DevCommitHash
	case string(metadata.EnvStaging):
		commitHash = version.StagingCommitHash
	case string(metadata.EnvProd):
		commitHash = version.ProdCommitHash
	default:
		commitHash = version.DevCommitHash
	}

	// Find protected image that matches the config db commit hash
	for _, image := range protectedFromScaleDown {
		if image.ClientSHA == commitHash {
			newImage = image
			newImageID = image.ImageID
			break
		}
	}

	if newImage == (subscriptions.Image{}) {
		return utils.MakeError("did not find protected image with commit hash %v. Not performing image swapover.", commitHash)
	}

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

	// swapover active image on database
	logger.Infof("Updating old %v image %v to new image %v on database.", event.Region, oldImageID, newImageID)

	if oldImageID == "" {
		_, err = s.DBClient.InsertImages(scalingCtx, s.GraphQLClient, []subscriptions.Image{newImage})
		if err != nil {
			return utils.MakeError("Failed to insert image %v into database. Error: %v", newImageID, err)
		}
	} else {
		_, err = s.DBClient.UpdateImage(scalingCtx, s.GraphQLClient, newImage)
		if err != nil {
			return utils.MakeError("Failed to update image %v to image %v in database. Error: %v", oldImageID, newImageID, err)
		}
	}

	// Unprotect the image until we have successfully swapped images in database
	delete(protectedFromScaleDown, newImage.ImageID)

	return nil
}
