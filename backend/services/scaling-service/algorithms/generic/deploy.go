package generic

import (
	"context"
	"reflect"
	"time"

	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/scaling-service/algorithms"
	"github.com/whisthq/whist/backend/services/scaling-service/algorithms/helpers"
	"github.com/whisthq/whist/backend/services/subscriptions"
	"github.com/whisthq/whist/backend/services/utils"
	logger "github.com/whisthq/whist/backend/services/whistlogger"
)

// UpgradeImage is a scaling action which runs when a new version is deployed. Its responsible of
// starting a buffer of instances with the new image and scaling down instances with the previous
// image.
func (s *GenericScalingAlgorithm) UpgradeImage(scalingCtx context.Context, event algorithms.ScalingEvent, imageID interface{}) error {
	logger.Infof("Starting upgrade image action for event: %v", event)
	defer logger.Infof("Finished upgrade image action for event: %v", event)

	// Check if we received a valid image before performing more
	// expensive operations.

	if imageID == nil {
		logger.Warningf("Received an empty image ID on %v. Not performing upgrade.", event.Region)
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
		return utils.MakeError("failed to query database for current image on %v. Err: %v", event.Region, err)
	}

	logger.Infof("Creating new instance buffer for image %v", newImage.ImageID)

	// Slice that will hold the instances and pass them to the dbclient
	var instancesForDb []subscriptions.Instance

	// If we are running on a local or testing environment, spinup "fake" instances to avoid
	// creating them on a cloud provider. In any other case we call the host handler to create
	// them on the cloud provider for us.
	if metadata.IsLocalEnv() && !metadata.IsRunningInCI() {
		logger.Infof("Running on localdev so scaling up fake instances.")
		instancesForDb = helpers.SpinUpFakeInstances(defaultInstanceBuffer, newImage.ImageID, event.Region)
	} else {
		bufferInstances, err := s.Host.SpinUpInstances(scalingCtx, int32(defaultInstanceBuffer), maxWaitTimeReady, newImage)
		if err != nil {
			return utils.MakeError("failed to create instance buffer for image %v. Error: %v", newImage.ImageID, err)
		}

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
	}

	logger.Infof("Inserting newly created instances to database.")

	// If successful, write to database
	affectedRows, err := s.DBClient.InsertInstances(scalingCtx, s.GraphQLClient, instancesForDb)
	if err != nil {
		return utils.MakeError("Failed to insert instances into database. Error: %v", err)
	}

	logger.Infof("Inserted %v rows to database.", affectedRows)

	// Acquire lock on protected from scale down map
	s.protectedMapLock.Lock()
	defer s.protectedMapLock.Unlock()

	// Protect the new instance buffer from scale down. This is done to avoid any downtimes
	// during deploy, as the active image will be switched until the client app has updated
	// its version on the config database.
	s.protectedFromScaleDown = make(map[string]subscriptions.Image)
	s.protectedFromScaleDown[newImage.ImageID] = newImage

	// If the region does not have an existing image, insert the new one to the database.
	if len(imageResult) == 0 {
		logger.Warningf("Image doesn't exist on %v. Creating a new entry with image %v.", event.Region, newImage.ImageID)

		updateParams := subscriptions.Image{
			Provider:  "AWS",
			Region:    event.Region,
			ImageID:   newImage.ImageID,
			ClientSHA: newImage.ClientSHA,
			UpdatedAt: time.Now(),
		}

		_, err = s.DBClient.InsertImages(scalingCtx, s.GraphQLClient, []subscriptions.Image{updateParams})
		if err != nil {
			return utils.MakeError("Failed to insert image %v into database. Error: %v", newImage.ImageID, err)
		}
	}

	// Notify through the synchan that the image upgrade is done
	// so that we can continue to swapover images when the config
	// database updates. We time out here in case the frontend build
	// failed to deploy, and if it does we rollback the new version.
	select {
	case s.SyncChan <- true:
		logger.Infof("Finished upgrading image %v in region %v", newImage.ImageID, event.Region)
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
func (s *GenericScalingAlgorithm) SwapOverImages(scalingCtx context.Context, event algorithms.ScalingEvent, clientVersion interface{}) error {
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

// Helper functions

// We need to use a custom compare function to compare protectedFromScaleDown maps
// because the `UpdatedAt` is a timestamp set with `time.Now`.
func compareProtectedMaps(a map[string]subscriptions.Image, b map[string]subscriptions.Image) bool {
	var equal bool

	if (len(a) == 0) && (len(b) == 0) {
		equal = true
	}

	for k, v := range a {
		for s, i := range b {
			if k != s {
				break
			}
			i.UpdatedAt = v.UpdatedAt
			equal = reflect.DeepEqual(v, i)
		}
	}

	return equal
}

// We need to use a custom compare function to compare WhistImages objects
// because the `UpdatedAt` is a timestamp set with `time.Now`.
func compareWhistImages(a subscriptions.WhistImages, b subscriptions.WhistImages) bool {
	var equal bool
	for _, v := range a {
		for _, i := range b {
			i.UpdatedAt = v.UpdatedAt
			equal = reflect.DeepEqual(v, i)
		}
	}

	return equal
}
