package scaling_algorithms

import (
	"context"
	"time"

	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/scaling-service/scaling_algorithms/helpers"
	"github.com/whisthq/whist/backend/services/subscriptions"
	"github.com/whisthq/whist/backend/services/utils"
	logger "github.com/whisthq/whist/backend/services/whistlogger"
	"go.uber.org/zap"
)

// deployTimeoutInHours indicates the time the scaling service
// should expect the deploy action to take. While this might seem
// like a high value, we need to set it to a sufficiently high value
// to give enough time to the deploy action to complete. This value
// is then used to take specific actions when a deploy has failed
// (rollback to the previous version) or succeeded (update the database).
const deployTimeoutInHours = 3 * time.Hour

// UpgradeImage is a scaling action which runs when a new version is deployed. Its responsible of
// starting a buffer of instances with the new image and scaling down instances with the previous
// image.
func (s *DefaultScalingAlgorithm) UpgradeImage(scalingCtx context.Context, event ScalingEvent, imageID interface{}) error {
	contextFields := []interface{}{
		zap.String("id", event.ID),
		zap.Any("type", event.Type),
		zap.String("region", event.Region),
	}
	logger.Infow("Starting upgrade image action.", contextFields)
	defer logger.Infow("Finished upgrade image action.", contextFields)

	// Check if we received a valid image before performing more
	// expensive operations.

	if imageID == nil {
		logger.Warningf("Received an empty image ID in %s. Not performing upgrade.", event.Region)
		return nil
	}

	newImage := subscriptions.Image{
		Provider:  "AWS",
		Region:    event.Region,
		ImageID:   imageID.(string),
		ClientSHA: metadata.GetGitCommit(),
		UpdatedAt: time.Now(),
	}

	// Query for the current image id
	imageResult, err := s.DBClient.QueryImage(scalingCtx, s.GraphQLClient, "AWS", event.Region)
	if err != nil {
		return utils.MakeError("failed to query database for current image: %s", err)
	}

	logger.Infow(utils.Sprintf("Creating new instance buffer for image %s", newImage.ImageID), contextFields)

	// Slice that will hold the instances and pass them to the dbclient
	var instancesForDb []subscriptions.Instance

	// If we are running on a local or testing environment, spinup "fake" instances to avoid
	// creating them on a cloud provider. In any other case we call the host handler to create
	// them on the cloud provider for us.
	if metadata.IsLocalEnv() && !metadata.IsRunningInCI() {
		logger.Infow("Running on localdev so scaling up fake instances.", contextFields)
		instancesForDb = helpers.SpinUpFakeInstances(defaultInstanceBuffer, newImage.ImageID, event.Region)
	} else {
		instancesForDb, err = s.Host.SpinUpInstances(scalingCtx, int32(defaultInstanceBuffer), maxWaitTimeReady, newImage)
		if err != nil {
			return utils.MakeError("failed to create instance buffer: %s", err)
		}

		// Set the instance capacity field and add to the slice
		// that will be passed to the database.
		for i := 0; i < len(instancesForDb); i++ {
			instancesForDb[i].RemainingCapacity = int64(instanceCapacity[instancesForDb[i].Type])
		}
	}

	logger.Infow("Inserting newly created instances to database.", contextFields)

	// If successful, write to database
	affectedRows, err := s.DBClient.InsertInstances(scalingCtx, s.GraphQLClient, instancesForDb)
	if err != nil {
		return utils.MakeError("failed to insert instances into database: %s", err)
	}

	logger.Infow(utils.Sprintf("Inserted %d rows to database.", affectedRows), contextFields)

	// Acquire lock on protected from scale down map
	s.protectedMapLock.Lock()
	defer s.protectedMapLock.Unlock()

	// Protect the new instance buffer from scale down. This is done to avoid any downtimes
	// during deploy, as the active image will be switched until the frontend has updated
	// its version on the config database.
	s.protectedFromScaleDown = make(map[string]subscriptions.Image)
	s.protectedFromScaleDown[newImage.ImageID] = newImage

	// If the region does not have an existing image, insert the new one to the database.
	if len(imageResult) == 0 {
		logger.Warningf("Image doesn't exist in %s. Creating a new entry with image %s.", event.Region, newImage.ImageID)

		updateParams := subscriptions.Image{
			Provider:  "AWS",
			Region:    event.Region,
			ImageID:   newImage.ImageID,
			ClientSHA: newImage.ClientSHA,
			UpdatedAt: time.Now(),
		}

		_, err = s.DBClient.InsertImages(scalingCtx, s.GraphQLClient, []subscriptions.Image{updateParams})
		if err != nil {
			return utils.MakeError("failed to insert image into database: %s", err)
		}
	}

	// Notify through the synchan that the image upgrade is done
	// so that we can continue to swapover images when the config
	// database updates. We time out here in case the frontend build
	// failed to deploy, and if it does we rollback the new version.
	select {
	case s.SyncChan <- true:
		logger.Infow(utils.Sprintf("Finished upgrading image %s in region %s", newImage.ImageID, event.Region), contextFields)
	case <-time.After(1 * time.Hour):
		// Clear protected map since the frontend deploy didn't complete successfully.
		s.protectedFromScaleDown = make(map[string]subscriptions.Image)

		return utils.MakeError("timed out waiting for config database to swap versions")
	}

	return nil
}

// SwapOverImages is a scaling action that will switch the current image on the given region.
// To the latest one. This is done separately to avoid having downtimes during deploys, since
// we have to wait until the frontend has updated its version on the config database.
func (s *DefaultScalingAlgorithm) SwapOverImages(scalingCtx context.Context, event ScalingEvent, clientVersion interface{}) error {
	contextFields := []interface{}{
		zap.String("id", event.ID),
		zap.Any("type", event.Type),
		zap.String("region", event.Region),
	}
	// Block until the image upgrade has finished successfully.
	// We time out here in case something went wrong with the
	// upgrade image action, in which case we roll back the new version.
	select {
	case <-s.SyncChan:
		logger.Infow("Got signal that image upgrade action finished correctly.", contextFields)
	case <-time.After(deployTimeoutInHours):
		// Clear protected map since the image upgrade didn't complete successfully.
		s.protectedMapLock.Lock()
		s.protectedFromScaleDown = make(map[string]subscriptions.Image)
		s.protectedMapLock.Unlock()

		return utils.MakeError("timed out waiting for image upgrade to finish.")
	}

	logger.Infow("Starting image swapover action.", contextFields)
	defer logger.Infow("Finished image swapover action.", contextFields)

	// version is the entry we receive from the config database
	version := clientVersion.(subscriptions.FrontendVersion)

	// Update the internal version with the new one received from the database.
	// This function updates the value inside the config file, so we can keep  track
	// of the current version locally, it does not update the value in the database.
	setFrontendVersion(version)

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
		return utils.MakeError("did not find protected image with commit hash %s. Not performing image swapover.", commitHash)
	}

	// Query for the current image id
	imageResult, err := s.DBClient.QueryImage(scalingCtx, s.GraphQLClient, "AWS", event.Region)
	if err != nil {
		return utils.MakeError("failed to query database for current image: %s", err)
	}

	if len(imageResult) == 0 {
		logger.Warningf("Image doesn't exist in %s. Creating a new entry with image %s.", event.Region, newImageID)
	} else {
		// We now consider the "current" image as the "old" image
		oldImageID = string(imageResult[0].ImageID)
	}

	// swapover active image on database
	logger.Infow(utils.Sprintf("Updating old %s image %s to new image %s on database.", event.Region, oldImageID, newImageID), contextFields)

	if oldImageID == "" {
		_, err = s.DBClient.InsertImages(scalingCtx, s.GraphQLClient, []subscriptions.Image{newImage})
		if err != nil {
			return utils.MakeError("failed to insert image into database: %s", err)
		}
	} else {
		_, err = s.DBClient.UpdateImage(scalingCtx, s.GraphQLClient, newImage)
		if err != nil {
			return utils.MakeError("failed to update image in database: %s", err)
		}
	}

	// Unprotect the image until we have successfully swapped images in database
	delete(s.protectedFromScaleDown, newImage.ImageID)

	return nil
}
