package scaling_algorithms

import (
	"context"

	"github.com/whisthq/whist/backend/services/scaling-service/scaling_algorithms/helpers"
	"github.com/whisthq/whist/backend/services/subscriptions"
	"github.com/whisthq/whist/backend/services/utils"
	logger "github.com/whisthq/whist/backend/services/whistlogger"
	"go.uber.org/zap"
)

// VerifyInstanceScaleDown is a scaling action which fires when an instance is marked as DRAINING
// on the database. Its purpose is to verify and wait until the instance is terminated from the
// cloud provider and removed from the database, if it doesn't it takes the necessary steps to
// notify and ensure the database and the cloud provider don't get out of sync.
func (s *DefaultScalingAlgorithm) VerifyInstanceScaleDown(scalingCtx context.Context, event ScalingEvent, instance subscriptions.Instance) error {
	contextFields := []interface{}{
		zap.String("id", event.ID),
		zap.Any("type", event.Type),
		zap.String("region", event.Region),
	}
	logger.Infow("Starting verify scale down action.", contextFields)
	defer logger.Infow("Finished verify scale down action.", contextFields)

	// We want to verify if we have the desired capacity after verifying scale down.
	defer func() {
		err := s.VerifyCapacity(scalingCtx, event)
		if err != nil {
			logger.Error(err)
		}
	}()

	// First, verify if the draining instance has mandelboxes running
	instanceResult, err := s.DBClient.QueryInstance(scalingCtx, s.GraphQLClient, instance.ID)
	if err != nil {
		return utils.MakeError("failed to query database for instance %s: %s", instance.ID, err)
	}

	for _, instanceRow := range instanceResult {
		// Compute how many mandelboxes are running on the instance. We use the current remaining capacity
		// and the total capacity of the instance type to check if it has running mandelboxes.
		usage := instanceCapacity[string(instanceRow.Type)] - instanceRow.RemainingCapacity
		if usage > 0 {
			logger.Infow(utils.Sprintf("Not scaling down draining instance because it has %d running mandelboxes.", usage), contextFields)
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
	contextFields := []interface{}{
		zap.String("id", event.ID),
		zap.Any("type", event.Type),
		zap.String("region", event.Region),
	}
	logger.Infow("Starting verify capacity action.", contextFields)
	defer logger.Infow("Finished verify capacity action.", contextFields)

	// TODO: set different cloud provider when doing multi-cloud
	latestImage, err := s.DBClient.QueryLatestImage(scalingCtx, s.GraphQLClient, "AWS", event.Region)
	if err != nil {
		return err
	}

	// This query will return all instances with the ACTIVE status
	allActive, err := s.DBClient.QueryInstancesByStatusOnRegion(scalingCtx, s.GraphQLClient, "ACTIVE", event.Region)
	if err != nil {
		return utils.MakeError("failed to query database for active instances: %s", err)
	}

	// This query will return all instances with the PRE_CONNECTION status
	allStarting, err := s.DBClient.QueryInstancesByStatusOnRegion(scalingCtx, s.GraphQLClient, "PRE_CONNECTION", event.Region)
	if err != nil {
		return utils.MakeError("failed to query database for starting instances: %s", err)
	}

	mandelboxCapacity := helpers.ComputeExpectedMandelboxCapacity(string(latestImage.ImageID), allActive, allStarting)

	// We consider the expected mandelbox capacity (active instances + starting instances)
	// to account for warmup time and so that we don't scale up unnecesary instances.
	if mandelboxCapacity < int64(desiredFreeMandelboxesPerRegion[event.Region]) {
		logger.Infow(utils.Sprintf("Current mandelbox capacity of %d is less than desired %d. Scaling up %d instances to satisfy minimum desired capacity.",
			mandelboxCapacity, desiredFreeMandelboxesPerRegion[event.Region], defaultInstanceBuffer), contextFields)
		err = s.ScaleUpIfNecessary(defaultInstanceBuffer, scalingCtx, event, latestImage)
		if err != nil {
			return err
		}
	} else {
		logger.Infow(utils.Sprintf("Mandelbox capacity %d in %s is enough to satisfy minimum desired capacity of %d.",
			mandelboxCapacity, event.Region, desiredFreeMandelboxesPerRegion[event.Region]), contextFields)
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
		return utils.MakeError("failed to query database for instance %s: %s", instance.ID, err)
	}

	// Verify that instance removed itself from the database
	if len(instanceResult) == 0 {
		logger.Infof("Instance %s was successfully removed from database.", instance.ID)
		return nil
	}

	// If instance still exists on the database, delete as it no longer exists on cloud provider.
	// This edge case means something went wrong with shutting down the instance and now the db is
	// out of sync with the cloud provider. To amend this, delete the instance and its mandelboxes
	// from the database.
	logger.Infof("Removing instance %s from database as it no longer exists on cloud provider.", instance.ID)

	affectedRows, err := s.DBClient.DeleteInstance(scalingCtx, s.GraphQLClient, instance.ID)
	if err != nil {
		return utils.MakeError("failed to delete instance %s from database: %s", instance.ID, err)
	}

	logger.Infof("Deleted %d rows from database.", affectedRows)
	return nil
}
