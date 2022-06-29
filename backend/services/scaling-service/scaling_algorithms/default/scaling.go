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

// ScaleDownIfNecessary is a scaling action which runs every 10 minutes and scales down free and
// lingering instances, respecting the buffer defined for each region. Free instances will be
// marked as draining, and lingering instances will be terminated and removed from the database.
func (s *DefaultScalingAlgorithm) ScaleDownIfNecessary(scalingCtx context.Context, event ScalingEvent) error {
	contextFields := []interface{}{
		zap.String("id", event.ID),
		zap.Any("type", event.Type),
		zap.String("region", event.Region),
	}
	logger.Infow("Starting scale down action.", contextFields)
	defer logger.Infow("Finished scale down action.", contextFields)

	// We want to verify if we have the desired capacity after scaling down.
	defer func() {
		err := s.VerifyCapacity(scalingCtx, event)
		if err != nil {
			logger.Errorf("error verifying capacity when scaling down: %s", err)
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
		return utils.MakeError("failed to query database for current image: %s", err)
	}

	if len(imageResult) == 0 {
		logger.Warningf("Image not found in %s. Not performing any scaling actions.", event.Region)
		return nil
	}
	latestImageID := string(imageResult[0].ImageID)

	// Query database for all active instances (status ACTIVE) without mandelboxes
	allActive, err := s.DBClient.QueryInstancesByStatusOnRegion(scalingCtx, s.GraphQLClient, "ACTIVE", event.Region)
	if err != nil {
		return utils.MakeError("failed to query database for active instances: %s", err)
	}

	// Active instances with space to run mandelboxes
	mandelboxCapacity := helpers.ComputeRealMandelboxCapacity(latestImageID, allActive)

	// Extra capacity is considered once we have a full instance worth of capacity
	// more than the desired free mandelboxes. TODO: Set the instance type once we
	// have support for more instance types. For now default to `g4dn.2xlarge`.
	extraCapacity := int64(desiredFreeMandelboxesPerRegion[event.Region]) + (int64(defaultInstanceBuffer) * instanceCapacity["g4dn.2xlarge"])

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
	for _, instance := range allActive {
		// Compute how many mandelboxes are running on the instance. We use the current remaining capacity
		// and the total capacity of the instance type to check if it has running mandelboxes.
		usage := instanceCapacity[string(instance.Type)] - instance.RemainingCapacity
		if usage > 0 {
			// Don't scale down any instance that has running
			// mandelboxes, regardless of the image it uses
			logger.Infow(utils.Sprintf("Not scaling down instance %s because it has %d mandelboxes running.", instance.ID, usage), contextFields)
			continue
		}

		_, protected := s.protectedFromScaleDown[instance.ImageID]
		if protected {
			// Don't scale down instances with a protected image id. A protected
			// image id refers to the image id that was built and passed by the
			// `build-and-deploy` workflow, that is waiting for the config database
			// to update commit hashes. For this reason, we don't scale down the
			// instance buffer created for this image, instead we "protect" it until
			// its ready to use, to avoid downtimes and to create the buffer only once.
			logger.Infow(utils.Sprintf("Not scaling down instance %s because it has an image id that is protected from scale down.", instance.ID), contextFields)
			continue
		}

		if instance.ImageID == latestImageID {
			// Current instances
			// If we have more than one instance worth of extra mandelbox capacity, scale down
			if mandelboxCapacity >= extraCapacity {
				logger.Infow(utils.Sprintf("Scaling down instance %s because we have more mandelbox capacity of %d than desired %d.", instance.ID, mandelboxCapacity, extraCapacity), contextFields)
				freeInstances = append(freeInstances, instance)
				mandelboxCapacity -= instance.RemainingCapacity
			}
		} else {
			// Old instances
			logger.Infow(utils.Sprintf("Scaling down instance %s because it has an old image id %s.", instance.ID, instance.ImageID), contextFields)
			freeInstances = append(freeInstances, instance)
		}
	}

	// Check database for draining instances without running mandelboxes
	drainingInstances, err := s.DBClient.QueryInstancesByStatusOnRegion(scalingCtx, s.GraphQLClient, "DRAINING", event.Region)
	if err != nil {
		return utils.MakeError("failed to query database for lingering instances: %s", err)
	}

	for _, drainingInstance := range drainingInstances {
		// Check if lingering instance has any running mandelboxes
		if drainingInstance.RemainingCapacity == 0 {
			// If not, notify, could be a stuck mandelbox (check if mandelbox is > day old?)
			logger.Warningf("Instance %s has associated mandelboxes and is marked as Draining.", drainingInstance.ID)
		} else if time.Since(drainingInstance.UpdatedAt) > 10*time.Minute {
			// Mark instance as lingering only if it has taken more than 10 minutes to shut down.
			lingeringInstances = append(lingeringInstances, drainingInstance)
			lingeringIDs = append(lingeringIDs, string(drainingInstance.ID))
		}
	}

	// If there are any lingering instances, try to shut them down on AWS. This is a fallback measure for when an instance
	// fails to receive a drain and shutdown event from the pubsub (i.e. the Hasura server restarted or went down).
	if len(lingeringInstances) > 0 && !metadata.IsLocalEnv() {
		logger.Infow(utils.Sprintf("Terminating %d lingering instances in %s", len(lingeringInstances), event.Region), contextFields)

		err := s.Host.SpinDownInstances(scalingCtx, lingeringIDs)
		if err != nil {
			return utils.MakeError("failed to terminate lingering instances: %s", err)
		}

		// Verify that each instance is terminated correctly and is removed from the database
		for _, lingeringInstance := range lingeringInstances {
			err = s.VerifyInstanceRemoval(scalingCtx, lingeringInstance)
			if err != nil {
				return err
			}
		}

	} else if !metadata.IsLocalEnv() {
		logger.Infow(utils.Sprintf("There are no lingering instances in %s.", event.Region), contextFields)
	} else {
		logger.Infow("Running on localdev so not spinning down lingering instances.", contextFields)

		// Verify that each instance is terminated correctly and is removed from the database
		for _, lingeringInstance := range lingeringInstances {
			err = s.VerifyInstanceRemoval(scalingCtx, lingeringInstance)
			if err != nil {
				return err
			}
		}
	}

	// Verify if there are free instances that can be scaled down
	if len(freeInstances) == 0 {
		logger.Infow(utils.Sprintf("There are no free instances to scale down in %s.", event.Region), contextFields)
		return nil
	}

	logger.Infow(utils.Sprintf("Scaling down %d free instances on %s.", len(freeInstances), event.Region), contextFields)

	for _, instance := range freeInstances {
		logger.Infow(utils.Sprintf("Scaling down instance %s.", instance.ID), contextFields)

		instance.Status = "DRAINING"
		_, err = s.DBClient.UpdateInstance(scalingCtx, s.GraphQLClient, instance)
		if err != nil {
			return utils.MakeError("failed to mark instance %s as draining: %s", instance.ID, err)
		}

		logger.Infow(utils.Sprintf("Marked instance %s as draining on database.", instance.ID), contextFields)
	}

	return err
}

// ScaleUpIfNecessary is a scaling action that launched the received number of instances on
// the cloud provider and registers them on the database with the initial values.
func (s *DefaultScalingAlgorithm) ScaleUpIfNecessary(instancesToScale int, scalingCtx context.Context, event ScalingEvent, image subscriptions.Image) error {
	contextFields := []interface{}{
		zap.String("id", event.ID),
		zap.Any("type", event.Type),
		zap.String("region", event.Region),
	}
	logger.Infow("Starting scale up action.", contextFields)
	defer logger.Infow("Finished scale up action.", contextFields)

	// Try scale up in given region
	instanceNum := int32(instancesToScale)

	// Slice that will hold the instances and pass them to the dbclient
	var instancesForDb []subscriptions.Instance

	// If we are running on a local or testing environment, spinup "fake" instances to avoid
	// creating them on a cloud provider. In any other case we call the host handler to create
	// them on the cloud provider for us.
	if metadata.IsLocalEnv() && !metadata.IsRunningInCI() {
		logger.Infow("Running on localdev so scaling up fake instances.", contextFields)
		instancesForDb = helpers.SpinUpFakeInstances(instancesToScale, image.ImageID, event.Region)
	} else {
		// Call the host handler to handle the instance spinup in the cloud provider
		createdInstances, err := s.Host.SpinUpInstances(scalingCtx, instanceNum, maxWaitTimeReady, image)
		if err != nil {
			return utils.MakeError("failed to spin up instances: %s", err)
		}

		// Check if we could create the desired number of instances
		if len(createdInstances) != instancesToScale {
			return utils.MakeError("could not scale up %d instances, only scaled up %d", instancesToScale, len(createdInstances))
		}

		for _, instance := range createdInstances {
			instance.RemainingCapacity = int64(instanceCapacity[instance.Type])
			instancesForDb = append(instancesForDb, instance)
			logger.Infow(utils.Sprintf("Created tagged instance with ID %s", instance.ID), contextFields)
		}
	}

	logger.Infow("Inserting newly created instances to database.", contextFields)

	// If successful, write to database
	affectedRows, err := s.DBClient.InsertInstances(scalingCtx, s.GraphQLClient, instancesForDb)
	if err != nil {
		return utils.MakeError("failed to insert instances into database: %s", err)
	}

	logger.Infow(utils.Sprintf("Inserted %d rows to database.", affectedRows), contextFields)

	return nil
}
