package scaling_algorithms

import (
	"context"
	"time"

	"github.com/google/uuid"
	"github.com/whisthq/whist/backend/services/httputils"
	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/scaling-service/scaling_algorithms/helpers"
	"github.com/whisthq/whist/backend/services/subscriptions"
	"github.com/whisthq/whist/backend/services/types"
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
	if mandelboxCapacity < desiredFreeMandelboxesPerRegion {
		logger.Infof("Current mandelbox capacity of %v is less than desired %v. Scaling up %v instances to satisfy minimum desired capacity.", mandelboxCapacity, desiredFreeMandelboxesPerRegion, defaultInstanceBuffer)
		err = s.ScaleUpIfNecessary(defaultInstanceBuffer, scalingCtx, event, latestImageID)
		if err != nil {
			// err is already wrapped here
			return err
		}
	} else {
		logger.Infof("Mandelbox capacity %v in %v is enough to satisfy minimum desired capacity of %v.", mandelboxCapacity, event.Region, desiredFreeMandelboxesPerRegion)
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
		freeInstances, lingeringInstances []subscriptions.Instance
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

	// Query database for all active instances (status ACTIVE) without mandelboxes
	allActive, err := s.DBClient.QueryInstancesByStatusOnRegion(scalingCtx, s.GraphQLClient, "ACTIVE", event.Region)
	if err != nil {
		return utils.MakeError("failed to query database for active instances. Err: %v", err)
	}

	// Active instances with space to run mandelboxes
	mandelboxCapacity := helpers.ComputeRealMandelboxCapacity(latestImageID, allActive)

	// Extra capacity is considered once we have a full instance worth of capacity
	// more than the desired free mandelboxes. TODO: Set the instance type once we
	// have support for more instance types. For now default to `g4dn.2xlarge`.
	extraCapacity := desiredFreeMandelboxesPerRegion + (defaultInstanceBuffer * instanceCapacity["g4dn.2xlarge"])

	// Acquire lock on protected from scale down map
	s.protectedMapLock.Lock()
	defer s.protectedMapLock.Unlock()

	// Create a list of instances that can be scaled down from the active instances list.
	// For this, we have to consider the following conditions:
	// 1. Does the instance have any running mandelboxes? If so, don't scale down.
	// 2. Does the instance have the latest image, corresponding to the latest entry on the database?
	// If so, check if we have more than the desired number of mandelbox capacity. In case we do, add the instance
	// to the list that will be scaled down.
	// 3. If the instance does not have the latest image, and is not running any mandelboxes, add to the
	// list that will be scaled down.
	for _, dbInstance := range allActive {
		if len(dbInstance.Mandelboxes) > 0 {
			// Don't scale down any instance that has running
			// mandelboxes, regardless of the image it uses
			logger.Infof("Not scaling down instance %v because it has %v mandelboxes running.", dbInstance.ID, len(dbInstance.Mandelboxes))
			continue
		}

		instance := subscriptions.Instance{
			ID:                string(dbInstance.ID),
			Provider:          string(dbInstance.Provider),
			Region:            string(dbInstance.Region),
			ImageID:           string(dbInstance.ImageID),
			ClientSHA:         string(dbInstance.ClientSHA),
			IPAddress:         dbInstance.IPAddress,
			Type:              string(dbInstance.Type),
			RemainingCapacity: int64(dbInstance.RemainingCapacity),
			Status:            string(dbInstance.Status),
			CreatedAt:         dbInstance.CreatedAt,
			UpdatedAt:         dbInstance.UpdatedAt,
		}

		_, protected := s.protectedFromScaleDown[instance.ImageID]
		if protected {
			// Don't scale down instances with a protected image id. A protected
			// image id refers to the image id that was built and passed by the
			// `build-and-deploy` workflow, that is waiting for the config database
			// to update commit hashes. For this reason, we don't scale down the
			// instance buffer created for this image, instead we "protect" it until
			// its ready to use, to avoid downtimes and to create the buffer only once.
			logger.Infof("Not scaling down instance %v because it has an image id that is protected from scale down.", instance.ID)
			continue
		}

		if instance.ImageID == latestImageID {
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

	// Check database for draining instances without running mandelboxes
	drainingInstances, err := s.DBClient.QueryInstancesByStatusOnRegion(scalingCtx, s.GraphQLClient, "DRAINING", event.Region)
	if err != nil {
		return utils.MakeError("failed to query database for lingering instances. Err: %v", err)
	}

	for _, dbInstance := range drainingInstances {
		instance := subscriptions.Instance{
			ID:                string(dbInstance.ID),
			Provider:          string(dbInstance.Provider),
			Region:            string(dbInstance.Region),
			ImageID:           string(dbInstance.ImageID),
			ClientSHA:         string(dbInstance.ClientSHA),
			IPAddress:         dbInstance.IPAddress,
			Type:              string(dbInstance.Type),
			RemainingCapacity: int64(dbInstance.RemainingCapacity),
			Status:            string(dbInstance.Status),
			CreatedAt:         dbInstance.CreatedAt,
			UpdatedAt:         dbInstance.UpdatedAt,
		}
		// Check if lingering instance is free from mandelboxes
		if len(dbInstance.Mandelboxes) == 0 {
			lingeringInstances = append(lingeringInstances, instance)
			lingeringIDs = append(lingeringIDs, string(instance.ID))
		} else {
			// If not, notify, could be a stuck mandelbox (check if mandelbox is > day old?)
			logger.Warningf("Instance %v has %v associated mandelboxes and is marked as Draining.", instance.ID, len(dbInstance.Mandelboxes))
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

		instance.Status = "DRAINING"
		_, err = s.DBClient.UpdateInstance(scalingCtx, s.GraphQLClient, instance)
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

	// If successful, write to database
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
	bufferInstances, err := s.Host.SpinUpInstances(scalingCtx, int32(defaultInstanceBuffer), maxWaitTimeReady, newImageID)
	if err != nil {
		return utils.MakeError("failed to create instance buffer for image %v. Error: %v", newImageID, err)
	}

	// Create slice of newly created instance IDs
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

	// If successful, write to database
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

	// Acquire lock on protected from scale down map
	s.protectedMapLock.Lock()
	defer s.protectedMapLock.Unlock()

	// Protect the new instance buffer from scale down. This is done to avoid any downtimes
	// during deploy, as the active image will be switched until the client app has updated
	// its version on the config database.
	s.protectedFromScaleDown = make(map[string]subscriptions.Image)
	s.protectedFromScaleDown[newImageID] = newImage

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

	// Notify through the synchan that the image upgrade is done
	// so that we can continue to swapover images when the config
	// database updates. We time out here in case the frontend build
	// failed to deploy, and if it does we rollback the new version.
	select {
	case s.SyncChan <- true:
		logger.Infof("Finished upgrading image %v in region %v", newImageID, event.Region)
	case <-time.After(1 * time.Hour):
		// Clear protected map since the client app deploy didn't complete successfully.
		s.protectedFromScaleDown = make(map[string]subscriptions.Image)

		return utils.MakeError("Timed out waiting for config database to swap versions. Rolling back deploy of new version.")
	}

	return nil
}

// SwapOverImages is a scaling action that will switch the current image on the given region.
// To the latest one. This is done separately to avoid having downtimes during deploys, since
// we have to wait until the frontend has updated its version on the config database.
func (s *DefaultScalingAlgorithm) SwapOverImages(scalingCtx context.Context, event ScalingEvent, clientVersion interface{}) error {
	// Block until the image upgrade has finished successfully.
	// We time out here in case something went wrong with the
	// upgrade image action, in which case we roll back the new version.
	select {
	case <-s.SyncChan:
		logger.Infof("Got signal that image upgrade action finished correctly.")
	case <-time.After(1 * time.Hour):
		// Clear protected map since the image upgrade didn't complete successfully.
		s.protectedMapLock.Lock()
		s.protectedFromScaleDown = make(map[string]subscriptions.Image)
		s.protectedMapLock.Unlock()

		return utils.MakeError("Timed out waiting for image upgrade to finish. Rolling back deploy of new version.")
	}

	logger.Infof("Starting image swapover action for event: %v", event)
	defer logger.Infof("Finished image swapover action for event: %v", event)

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

	// Acquire lock on protected from scale down map
	s.protectedMapLock.Lock()
	defer s.protectedMapLock.Unlock()

	// Find protected image that matches the config db commit hash
	for _, image := range s.protectedFromScaleDown {
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
	delete(s.protectedFromScaleDown, newImage.ImageID)

	return nil
}

// MandelboxAssign is the action responsible for assigning an instance to a user,
// and scaling as necessary to satisfy demand.
func (s *DefaultScalingAlgorithm) MandelboxAssign(scalingCtx context.Context, event ScalingEvent) error {
	logger.Infof("Starting mandelbox assign action for event: %v", event)
	defer logger.Infof("Finished mandelbox assign action for event: %v", event)

	// We want to verify if we have the desired capacity after assigning a mandelbox
	defer func() {
		err := s.VerifyCapacity(scalingCtx, event)
		if err != nil {
			logger.Errorf("Error verifying capacity on %v. Err: %v", event.Region, err)
		}
	}()

	mandelboxRequest := event.Data.(*httputils.MandelboxAssignRequest)

	// Note: we receive the email from the client, so its value should
	// not be trusted for anything else other than logging since
	// it can be spoofed. We sanitize the email before using to help mitigate
	// potential attacks.
	unsafeEmail, err := helpers.SanitizeEmail(mandelboxRequest.UserEmail)
	if err != nil {
		// err is already wrapped here
		return err
	}

	var (
		requestedRegions = mandelboxRequest.Regions
		allowedRegions   []string
		assignedInstance subscriptions.Instance
	)

	// Populate allowedRegions
	for _, enabledRegion := range GetEnabledRegions() {
		for _, requestedRegion := range requestedRegions {
			if enabledRegion == requestedRegion {
				allowedRegions = append(allowedRegions, requestedRegion)
			}
		}
	}

	if len(allowedRegions) == 0 {
		err := utils.MakeError("could not assign mandelbox. Wanted regions are not valid or are not enabled.")
		mandelboxRequest.ReturnResult(httputils.MandelboxAssignRequestResult{
			Error: REGION_NOT_ENABLED,
		}, err)
		return err
	}

	// This condition is to accomodate the worflow for developers of client_apps
	// to test their changes without needing to update the development database with
	// commit_hashes on their local machines.
	if metadata.IsLocalEnv() && mandelboxRequest.CommitHash == CLIENT_COMMIT_HASH_DEV_OVERRIDE {
		// Query for the latest image id
		imageResult, err := s.DBClient.QueryImage(scalingCtx, s.GraphQLClient, "AWS", event.Region) // TODO: set different provider when doing multi-cloud.
		if err != nil {
			return utils.MakeError("failed to query database for current image. Err: %v", err)
		}

		if len(imageResult) == 0 {
			return utils.MakeError("Image not found on %v.", event.Region)
		}

		mandelboxRequest.CommitHash = string(imageResult[0].ClientSHA)
	}

	// Start looking for instances
	for _, region := range allowedRegions {
		logger.Infof("Trying to find instance for user %v in region %v, with commit hash %v. (client reported email %v, this value might not be accurate and is untrusted)",
			unsafeEmail, region, mandelboxRequest.CommitHash, unsafeEmail)

		instanceResult, err := s.DBClient.QueryInstanceWithCapacity(scalingCtx, s.GraphQLClient, region)
		if err != nil {
			return utils.MakeError("failed to query for instance with capacity. Err: %v", err)
		}

		if len(instanceResult) == 0 {
			logger.Warningf("Failed to find an instance in %v for commit hash %v. Trying on next region.", region, mandelboxRequest.CommitHash)
			continue
		}

		assignedInstance = subscriptions.Instance{
			ID:                string(instanceResult[0].ID),
			IPAddress:         string(instanceResult[0].IPAddress),
			Provider:          string(instanceResult[0].Provider),
			Region:            string(instanceResult[0].Region),
			ImageID:           string(instanceResult[0].ImageID),
			ClientSHA:         string(instanceResult[0].ClientSHA),
			Type:              string(instanceResult[0].Type),
			RemainingCapacity: int64(instanceCapacity[string(instanceResult[0].Type)]),
			Status:            string(instanceResult[0].Status),
			CreatedAt:         instanceResult[0].CreatedAt,
			UpdatedAt:         instanceResult[0].UpdatedAt,
		}

		if assignedInstance.ClientSHA != mandelboxRequest.CommitHash {
			err := utils.MakeError("found instance with capacity but it has a different commit hash %v that frontend with commit hash  %v", assignedInstance.ClientSHA, mandelboxRequest.CommitHash)
			mandelboxRequest.ReturnResult(httputils.MandelboxAssignRequestResult{
				Error: COMMIT_HASH_MISMATCH,
			}, err)
			return err
		}

		logger.Infof("Found instance %v for user %v in %v", assignedInstance.ID, unsafeEmail, region)
		break
	}

	if assignedInstance == (subscriptions.Instance{}) {
		err := utils.MakeError("did not find an instance with capacity for user %v and commit hash %v.", mandelboxRequest.UserEmail, mandelboxRequest.CommitHash)
		mandelboxRequest.ReturnResult(httputils.MandelboxAssignRequestResult{
			Error: NO_INSTANCE_AVAILABLE,
		}, err)
		return err
	}

	mandelboxID, err := uuid.NewRandom()
	if err != nil {
		return utils.MakeError("failed to create a mandelbox id. Err: %v", err)
	}

	mandelboxesForDb := []subscriptions.Mandelbox{
		{
			ID:         types.MandelboxID(mandelboxID),
			App:        "CHROME",
			InstanceID: assignedInstance.ID,
			UserID:     mandelboxRequest.UserID,
			SessionID:  utils.Sprintf("%v", mandelboxRequest.SessionID),
			Status:     "ALLOCATED",
			CreatedAt:  time.Now(),
		},
	}

	// Allocate mandelbox on database, this will start the mandelbox inside the assigned instance
	affectedRows, err := s.DBClient.InsertMandelboxes(scalingCtx, s.GraphQLClient, mandelboxesForDb)
	if err != nil {
		return utils.MakeError("error while inserting mandelbox to database. Err: %v", err)
	}

	logger.Infof("Inserted %v rows to database.", affectedRows)

	if assignedInstance.RemainingCapacity <= 0 {
		// This should never happen, but we should consider
		// possible edge cases before updating the database.
		return utils.MakeError("instance with id %v has a remaning capacity less than or equal to 0.", assignedInstance.ID)
	}

	// Subtract 1 from the current instance capacity because we allocated a mandelbox
	updatedCapacity := assignedInstance.RemainingCapacity - 1
	assignedInstance.RemainingCapacity = updatedCapacity

	affectedRows, err = s.DBClient.UpdateInstance(scalingCtx, s.GraphQLClient, assignedInstance)
	if err != nil {
		return utils.MakeError("error while updating instance capacity on database. Err: %v", err)
	}

	logger.Infof("Updated %v rows in database.", affectedRows)

	// Return result to assign request
	mandelboxRequest.ReturnResult(httputils.MandelboxAssignRequestResult{
		IP:          assignedInstance.IPAddress,
		MandelboxID: types.MandelboxID(mandelboxID),
	}, nil)

	return nil
}
