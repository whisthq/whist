package helpers

import (
	"github.com/hasura/go-graphql-client"
	"github.com/whisthq/whist/backend/services/subscriptions"
)

// ComputeRealCapacity is a helper function to compute the actual capacity on a given region-image
// pair. This function takes into account the number of instances which can serve, as well as the
// number of mandelboxes they can run. We define real capacity as the number of instances which have
// space to run mandelboxes.
func ComputeRealCapacity(imageID string, activeInstances subscriptions.WhistInstances) int {
	var realCapacity int // The number of instances which are active and have space

	// Loop over active instances (status ACTIVE), only consider the ones with the current image
	for _, instance := range activeInstances {
		if instance.ImageID == graphql.String(imageID) &&
			instance.RemainingCapacity > 0 {
			realCapacity++
		}
	}

	return realCapacity
}

// ComputeExpectedCapacity is a helper function to compute the expected capacity on a given region-image
// pair. This function takes into account the number of instances which can serve, as well as the number
// of mandelboxes they can run. We define expected capacity as the number of instances in active state
// which have space to run mandelboxes added to the number of starting instances (PRE_CONNECTION status).
func ComputeExpectedCapacity(imageID string, activeInstances subscriptions.WhistInstances, startingInstances subscriptions.WhistInstances) int {
	var expectedCapacity int // The number of instances both in active and starting state

	realCapacity := ComputeRealCapacity(imageID, activeInstances)

	// Loop over starting instances (status PRE_CONNECTION, only consider the ones with the current image
	for _, instance := range startingInstances {
		if instance.ImageID == graphql.String(imageID) {
			expectedCapacity++
		}
	}

	return realCapacity + expectedCapacity
}
