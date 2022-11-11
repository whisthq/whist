package scaling

import (
	"context"
	"fmt"
	"time"

	"github.com/whisthq/whist/backend/elegant_monolith/internal"
	"github.com/whisthq/whist/backend/elegant_monolith/internal/config"
	"github.com/whisthq/whist/backend/elegant_monolith/pkg/logger"
)

func (svc scalingService) ScaleUpIfNecessary(ctx context.Context, instancesToScale int, imageID string, region string) ([]string, error) {
	logger.Infof("Starting scale up action.")
	defer logger.Infof("Finished scale up action.")

	var (
		// Slice that will hold the instances and pass them to the dbclient
		instancesForDb []internal.Instance
		// Slice that will hold fake mandelboxes and pass them to the dbclient
		fakeMandelboxesForDb []internal.Mandelbox
		err                  error
	)

	// If we are running on a local or testing environment, spinup "fake" instances to avoid
	// creating them on a cloud provider. In any other case we call the host handler to create
	// them on the cloud provider for us.
	if config.GetScalingEnabled() {
		logger.Infof("Scaling is disabled, so creating fake instances.")
		instancesForDb, fakeMandelboxesForDb = spinUpFakeInstances(instancesToScale, imageID, region)
	} else {
		// Call the host handler to handle the instance spinup in the cloud provider
		instancesForDb, err = svc.host.SpinUpInstances(ctx, int32(instancesToScale), maxWaitTimeReady, imageID)
		if err != nil {
			return nil, fmt.Errorf("failed to spin up instances: %s", err)
		}

		// Check if we could create the desired number of instances
		if len(instancesForDb) != instancesToScale {
			return nil, fmt.Errorf("could not scale up %d instances, only scaled up %d", instancesToScale, len(instancesForDb))
		}

		for i := 0; i < len(instancesForDb); i++ {
			instancesForDb[i].RemainingCapacity = instanceCapacity[instancesForDb[i].Type]
			logger.Infof("Created tagged instance with ID %s", instancesForDb[i].ID)
		}
	}

	logger.Infof("Inserting newly created instances to database.")

	// If successful, write to database
	affectedRows, err := svc.db.InsertInstances(ctx, instancesForDb)
	if err != nil {
		return nil, fmt.Errorf("failed to insert instances into database: %s", err)
	}

	logger.Infof("Inserted %d rows to database.", affectedRows)

	// This is to acommodate localdev testing flows in which we need to register fake mandelboxes
	// on the database.
	if len(fakeMandelboxesForDb) > 0 {
		affectedRows, err = svc.db.InsertMandelboxes(ctx, fakeMandelboxesForDb)
		if err != nil {
			return nil, fmt.Errorf("failed to insert fake mandelboxes to database: %s", err)
		}
		logger.Infof("Inserted %d rows to database.", affectedRows)
	}

	return []string{}, nil
}

func (svc scalingService) ScaleDownIfNecessary(ctx context.Context, region string) ([]string, error) {
	logger.Infof("Starting scale down action.")
	defer logger.Infof("Finished scale down action.")

	// Acceptable time for an instance to drain, if an instance takes longer it will be considered as lingering.
	const expectedDrainTime time.Duration = 10 * time.Minute

	var (
		// freeInstances is a list of instances with full-capacity available that have no running mandelboxes assigned to them.
		freeInstances []internal.Instance

		// lingeringInstances is a list of instances that have been draining for more time than expected.
		lingeringInstances []internal.Instance

		// drainingInstances is a list of instances that are currently draining, within the expected time.
		drainingInstances []internal.Instance

		// helper list to store lingering instance IDs.
		lingeringIDs []string

		err error
	)

	// TODO: set different cloud provider when doing multi-cloud
	latestImage, rows, err := svc.db.QueryLatestImage(ctx, "AWS", region)
	if err != nil {
		return nil, err
	}

	// Query database for all active instances (status ACTIVE) without mandelboxes
	allActive, rows, err := svc.db.QueryInstancesByStatusOnRegion(ctx, "ACTIVE", region)
	if err != nil {
		return nil, fmt.Errorf("failed to query database for active instances: %s", err)
	}

	// Active instances with space to run mandelboxes
	mandelboxCapacity := computeRealMandelboxCapacity(latestImage.ID, allActive)

	// Extra capacity is considered once we have a full instance worth of capacity
	// more than the desired free mandelboxes. TODO: Set the instance type once we
	// have support for more instance types. For now default to `g4dn.2xlarge`.
	targetFreeMandelboxes := config.GetTargetFreeMandelboxes(region)
	extraCapacity := targetFreeMandelboxes + (defaultInstanceBuffer * instanceCapacity["g4dn.2xlarge"])

	// Acquire lock on protected from scale down map
	//////////////////////////////////////////////
	// s.protectedMapLock.Lock()		    //
	// defer s.protectedMapLock.Unlock()	    //
	//////////////////////////////////////////////

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
			logger.Infof("Not scaling down instance %s because it has %d mandelboxes running.", instance.ID, usage)
			continue
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// _, protected := s.protectedFromScaleDown[instance.ImageID]										  //
		// if protected {															  //
		// 	// Don't scale down instances with a protected image id. A protected								  //
		// 	// image id refers to the image id that was built and passed by the								  //
		// 	// `build-and-deploy` workflow, that is waiting for the config database								  //
		// 	// to update commit hashes. For this reason, we don't scale down the								  //
		// 	// instance buffer created for this image, instead we "protect" it until							  //
		// 	// its ready to use, to avoid downtimes and to create the buffer only once.							  //
		// 	logger.Infof("Not scaling down instance %s because it has an image id that is protected from scale down.", instance.ID)		  //
		// 	continue															  //
		// }																	  //
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		if instance.ImageID == latestImage.ID {
			// Current instances
			// If we have more than one instance worth of extra mandelbox capacity, scale down
			if mandelboxCapacity >= extraCapacity {
				logger.Infof("Scaling down instance %s because we have more mandelbox capacity of %d than desired %d.", instance.ID, mandelboxCapacity, extraCapacity)
				freeInstances = append(freeInstances, instance)
				mandelboxCapacity -= instance.RemainingCapacity
			}
		} else {
			// Old instances
			logger.Infof("Scaling down instance %s because it has an old image id %s.", instance.ID, instance.ImageID)
			freeInstances = append(freeInstances, instance)
		}
	}

	// Check database for draining instances without running mandelboxes
	drainingInstances, rows, err = svc.db.QueryInstancesByStatusOnRegion(ctx, "DRAINING", region)
	if err != nil {
		return nil, fmt.Errorf("failed to query database for lingering instances: %s", err)
	}

	for _, drainingInstance := range drainingInstances {
		// Check if lingering instance has any running mandelboxes
		if drainingInstance.RemainingCapacity == 0 {
			// If not, notify, could be a stuck mandelbox (check if mandelbox is > day old?)
			logger.Warningf("Instance %s has associated mandelboxes and is marked as Draining.", drainingInstance.ID)
		} else if time.Since(drainingInstance.UpdatedAt) > expectedDrainTime {
			// Mark instance as lingering only if it has taken more than 10 minutes to shut down.
			lingeringInstances = append(lingeringInstances, drainingInstance)
			lingeringIDs = append(lingeringIDs, string(drainingInstance.ID))
		}
	}

	// If there are any lingering instances, try to shut them down on AWS. This is a fallback measure for when an instance
	// fails to receive a drain and shutdown event from the pubsub (i.e. the Hasura server restarted or went down).
	if len(lingeringInstances) > 0 && config.GetScalingEnabled() {
		logger.Infof("Terminating %d lingering instances in %s", len(lingeringInstances), region)

		err := svc.host.SpinDownInstances(ctx, lingeringIDs)
		if err != nil {
			return nil, fmt.Errorf("failed to terminate lingering instances: %s", err)
		}

		// Verify that each instance is terminated correctly and is removed from the database
		// for _, lingeringInstance := range lingeringInstances {
		// 	err = s.VerifyInstanceRemoval(ctx, lingeringInstance)
		// 	if err != nil {
		// 		return err
		// 	}
		// }

	} else if config.GetScalingEnabled() {
		logger.Infof("There are no lingering instances in %s.", region)
	} else {
		logger.Infof("Scaling is disabled so not spinning down lingering instances.")

		// Verify that each instance is terminated correctly and is removed from the database
		// for _, lingeringInstance := range lingeringInstances {
		// 	err = s.VerifyInstanceRemoval(ctx, lingeringInstance)
		// 	if err != nil {
		// 		return nil, err
		// 	}
		// }
	}

	// Verify if there are free instances that can be scaled down
	if len(freeInstances) == 0 {
		logger.Info("There are no free instances to scale down in %s.", region)
		return nil, nil
	}

	logger.Infof("Scaling down %d free instances in %s.", len(freeInstances), region)

	for _, instance := range freeInstances {
		logger.Infof("Scaling down instance %s.", instance.ID)

		instance.Status = "DRAINING"
		_, err = svc.db.UpdateInstance(ctx, instance)
		if err != nil {
			return nil, fmt.Errorf("failed to mark instance %s as draining: %s", instance.ID, err)
		}

		logger.Infof("Marked instance %s as draining on database.", instance.ID)
	}

	return []string{}, nil
}
