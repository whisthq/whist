package scaling_algorithms

import (
	"context"

	"github.com/whisthq/whist/backend/services/scaling-service/hosts"
	"github.com/whisthq/whist/backend/services/subscriptions"
)

func (s *DefaultScalingAlgorithm) VerifyInstanceScaleDown(scalingCtx context.Context, host hosts.HostHandler, event ScalingEvent, instance subscriptions.Instance) error {
	// First, verify if the draining instance has mandelboxes running

	// If instance has active mandelboxes, leave it alone

	// If not, wait until the instance is terminated from the cloud provider

	// Once its terminated, verify that it was removed from the database

	// Verify that instance removed itself from the database

	// If instance still exists on the database, forcefully delete as it no longer exists on cloud provider
	return nil

}

func (s *DefaultScalingAlgorithm) ScaleDownIfNecessary(scalingCtx context.Context, host hosts.HostHandler, event ScalingEvent) error {
	// check database for active instances without mandelboxes

	// Verify if there are free instances that can be scaled down

	// check database for draining instances without mandelboxes

	// Verify if there are lingering instances that can be scaled down

	// If so, forcefully remove terminnate and remove from database

	// If we have less instances than desired, try to scale up

	return nil
}

func (s *DefaultScalingAlgorithm) ScaleUpIfNecessary(instancesToScale int, scalingCtx context.Context, host hosts.HostHandler, event ScalingEvent, imageID string) error {
	// Try scale up in given region

	// Wait for instances to be ready on cloud provider

	// Check if we could create the desired number of instances

	// If successful, write to db
	return nil
}

func (s *DefaultScalingAlgorithm) UpgradeImage(scalingCtx context.Context, host hosts.HostHandler, event ScalingEvent, image subscriptions.Image) error {
	// create instance buffer with new image

	// wait for buffer to be ready

	// drain instances with old image

	// swapover active image on database
	return nil
}
