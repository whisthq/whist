package scaling_algorithms

import (
	"context"

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
		// Compute how many mandelboxes are running on the instance. We use the current remaining capacity
		// and the total capacity of the instance type to check if it has running mandelboxes.
		usage := instanceCapacity[string(instanceRow.Type)] - instanceRow.RemainingCapacity
		if usage > 0 {
			logger.Infof("Not scaling down draining instance because it has %v running mandelboxes.", usage)
			return nil
		}
	}

	err = s.VerifyInstanceRemoval(scalingCtx, instance)
	if err != nil {
		return err
	}

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
	latestImage := subscriptions.Image{
		Provider:  string(imageResult[0].Provider),
		Region:    string(imageResult[0].Region),
		ImageID:   string(imageResult[0].ImageID),
		ClientSHA: string(imageResult[0].ClientSHA),
		UpdatedAt: imageResult[0].UpdatedAt,
	}

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

	mandelboxCapacity := helpers.ComputeExpectedMandelboxCapacity(string(latestImage.ImageID), allActive, allStarting)

	// We consider the expected mandelbox capacity (active instances + starting instances)
	// to account for warmup time and so that we don't scale up unnecesary instances.
	if mandelboxCapacity < int64(desiredFreeMandelboxesPerRegion[event.Region]) {
		logger.Infof("Current mandelbox capacity of %v is less than desired %v. Scaling up %v instances to satisfy minimum desired capacity.", mandelboxCapacity, desiredFreeMandelboxesPerRegion[event.Region], defaultInstanceBuffer)
		err = s.ScaleUpIfNecessary(defaultInstanceBuffer, scalingCtx, event, latestImage)
		if err != nil {
			// err is already wrapped here
			return err
		}
	} else {
		logger.Infof("Mandelbox capacity %v in %v is enough to satisfy minimum desired capacity of %v.", mandelboxCapacity, event.Region, desiredFreeMandelboxesPerRegion[event.Region])
	}

	return nil
}

// VerifyInstanceRemoval is a function to verify that an instance terminates correctly on the cloud provider
// and to make sure it is removed from the database correctly. This keeps the database and cloud provider in
// sync.
func (s *DefaultScalingAlgorithm) VerifyInstanceRemoval(scalingCtx context.Context, instance subscriptions.Instance) error {
	// Wait until the host service terminates the instance in the cloud provider.
	err := s.Host.WaitForInstanceTermination(scalingCtx, maxWaitTimeTerminated, []string{instance.ID})
	if err != nil {
		return err
	}

	// Once its terminated, verify that it was removed from the database
	instanceResult, err := s.DBClient.QueryInstance(scalingCtx, s.GraphQLClient, instance.ID)
	if err != nil {
		return utils.MakeError("failed to query database for instance %v. Error: %v", instance.ID, err)
	}

	// Verify that instance removed itself from the database
	if len(instanceResult) == 0 {
		logger.Info("Instance %v was successfully removed from database.", instance.ID)
		return nil
	}

	// If instance still exists on the database, delete as it no longer exists on cloud provider.
	// This edge case means something went wrong with shutting down the instance and now the db is
	// out of sync with the cloud provider. To amend this, delete the instance and its mandelboxes
	// from the database.
	logger.Info("Removing instance %v from database as it no longer exists on cloud provider.", instance.ID)

	affectedRows, err := s.DBClient.DeleteInstance(scalingCtx, s.GraphQLClient, instance.ID)
	if err != nil {
		return utils.MakeError("failed to delete instance %v from database. Error: %v", instance.ID, err)
	}

	logger.Infof("Deleted %v rows from database.", affectedRows)
	return nil
}
