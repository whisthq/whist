package scaling_algorithms

import (
	"context"
	"time"

	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/scaling-service/scaling_algorithms/helpers"
	"github.com/whisthq/whist/backend/services/subscriptions"
	"github.com/whisthq/whist/backend/services/utils"
	logger "github.com/whisthq/whist/backend/services/whistlogger"
)

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
	extraCapacity := desiredFreeMandelboxesPerRegion[event.Region] + (defaultInstanceBuffer * instanceCapacity["g4dn.2xlarge"])

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
		// Compute how many mandelboxes are running on the instance. We use the current remaining capacity
		// and the total capacity of the instance type to check if it has running mandelboxes.
		usage := instanceCapacity[string(dbInstance.Type)] - int(dbInstance.RemainingCapacity)
		if usage > 0 {
			// Don't scale down any instance that has running
			// mandelboxes, regardless of the image it uses
			logger.Infof("Not scaling down instance %v because it has %v mandelboxes running.", dbInstance.ID, usage)
			continue
		}

		instance := subscriptions.WhistInstanceToInstance(dbInstance)

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
		instance := subscriptions.WhistInstanceToInstance(dbInstance)
		// Check if lingering instance has any running mandelboxes
		if dbInstance.RemainingCapacity == 0 {
			// If not, notify, could be a stuck mandelbox (check if mandelbox is > day old?)
			logger.Warningf("Instance %v has associated mandelboxes and is marked as Draining.", instance.ID)
		} else if time.Since(dbInstance.UpdatedAt) > 10*time.Minute {
			// Mark instance as lingering only if it has taken more than 10 minutes to shut down.
			lingeringInstances = append(lingeringInstances, instance)
			lingeringIDs = append(lingeringIDs, string(instance.ID))
		}
	}

	// If there are any lingering instances, try to shut them down on AWS. This is a fallback measure for when an instance
	// fails to receive a drain and shutdown event from the pubsub (i.e. the Hasura server restarted or went down).
	if len(lingeringInstances) > 0 && !metadata.IsLocalEnv() {
		logger.Infof("Terminating %v lingering instances in %s", len(lingeringInstances), event.Region)

		err := s.Host.SpinDownInstances(scalingCtx, lingeringIDs)
		if err != nil {
			return utils.MakeError("failed to terminate lingering instances. Err: %s", err)
		}

		// Verify that each instance is terminated correctly and is removed from the database
		for _, lingeringInstance := range lingeringInstances {
			err = s.VerifyInstanceRemoval(scalingCtx, lingeringInstance)
			if err != nil {
				return err
			}
		}

	} else if !metadata.IsLocalEnv() {
		logger.Info("There are no lingering instances in %v.", event.Region)
	} else {
		logger.Infof("Running on localdev so not spinning down lingering instances.")

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
func (s *DefaultScalingAlgorithm) ScaleUpIfNecessary(instancesToScale int, scalingCtx context.Context, event ScalingEvent, image subscriptions.Image) error {
	logger.Infof("Starting scale up action for event: %v", event)
	defer logger.Infof("Finished scale up action for event: %v", event)

	// Try scale up in given region
	instanceNum := int32(instancesToScale)

	// Slice that will hold the instances and pass them to the dbclient
	var instancesForDb []subscriptions.Instance

	// If we are running on a local or testing environment, spinup "fake" instances to avoid
	// creating them on a cloud provider. In any other case we call the host handler to create
	// them on the cloud provider for us.
	if metadata.IsLocalEnv() && !metadata.IsRunningInCI() {
		logger.Infof("Running on localdev so scaling up fake instances.")
		instancesForDb = helpers.SpinUpFakeInstances(instancesToScale, image.ImageID, event.Region)
	} else {
		// Call the host handler to handle the instance spinup in the cloud provider
		createdInstances, err := s.Host.SpinUpInstances(scalingCtx, instanceNum, maxWaitTimeReady, image)
		if err != nil {
			return utils.MakeError("Failed to spin up instances, created %v, err: %v", createdInstances, err)
		}

		// Check if we could create the desired number of instances
		if len(createdInstances) != instancesToScale {
			return utils.MakeError("Could not scale up %v instances, only scaled up %v.", instancesToScale, len(createdInstances))
		}

		for _, instance := range createdInstances {
			instance.RemainingCapacity = int64(instanceCapacity[instance.Type])
			instancesForDb = append(instancesForDb, instance)
			logger.Infof("Created tagged instance with ID %v", instance.ID)
		}
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
