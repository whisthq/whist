package helpers

import (
	"github.com/hasura/go-graphql-client"
	"github.com/whisthq/whist/backend/services/subscriptions"
)

// ComputeRealCapacity is a helper function to compute the actual capacity on a given region-image
// pair. This function takes into account the number of instances which can serve, as well as the
// number of mandelboxes they can run. We define real capacity as the number of instances which have
// space to run mandelboxes. This function returns the real capacity and the number of mandelboxes
// which can be started with the instance buffer.
func ComputeRealCapacity(imageID string, region string, activeInstances subscriptions.WhistInstances) (int, int) {
	var (
		realCapacity      int // The number of instances which are active and have space
		mandelboxCapacity int // The total number of mandelboxes that can be started
	)

	// Loop over active instances (status ACTIVE), only consider the ones with the current image
	for _, instance := range activeInstances {
		if instance.ImageID == graphql.String(imageID) &&
			instance.RemainingCapacity > 0 {
			realCapacity++
			mandelboxCapacity += int(instance.RemainingCapacity)
		}
	}

	return realCapacity, mandelboxCapacity
}

// ComputeExpectedCapacity is a helper function to compute the expected capacity on a given region-image
// pair. This function takes into account the number of instances which can serve, as well as the number
// of mandelboxes they can run. We define expected capacity as the number of instances in active state
// which have space to run mandelboxes and starting instances. This function returns the expected capacity
// and the number of mandelboxes which can be started with the instance buffer.
func ComputeExpectedCapacity(imageID string, region string, activeInstances subscriptions.WhistInstances, startingInstances subscriptions.WhistInstances) (int, int) {
	var (
		expectedCapacity  int // The number of instances both in active and starting state
		mandelboxCapacity int // The total number of mandelboxes that can be started
	)

	// Loop over active instances (status ACTIVE), only consider the ones with the current image
	for _, instance := range activeInstances {
		if instance.ImageID == graphql.String(imageID) &&
			instance.RemainingCapacity > 0 {
			expectedCapacity++
			mandelboxCapacity += int(instance.RemainingCapacity)
		}
	}

	// Loop over starting instances (status PRE_CONNECTION, only consider the ones with the current image
	for _, instance := range startingInstances {
		if instance.ImageID == graphql.String(imageID) {
			expectedCapacity++
		}
	}

	return expectedCapacity, mandelboxCapacity
}
