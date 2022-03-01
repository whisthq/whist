package helpers

import (
	"github.com/hasura/go-graphql-client"
	"github.com/whisthq/whist/backend/services/subscriptions"
)

// GetAvailableInstances is a helper function to get the number of instances that are
// active and have space to run mandelboxes.
func GetAvailableInstances(imageID string, activeInstances subscriptions.WhistInstances) int {
	var availableInstances int // The number of instances which are active and have space

	// Loop over active instances (status ACTIVE), only consider the ones with the current image
	for _, instance := range activeInstances {
		if instance.ImageID == graphql.String(imageID) &&
			instance.RemainingCapacity > 0 {
			availableInstances++
		}
	}

	return availableInstances
}

// GetExpectedInstances is a helper function to get the number of instances that are
// active and have space to run mandelboxes, summed with the number of instances that
// are on a starting state and will soon be active.
func GetExpectedInstances(imageID string, activeInstances subscriptions.WhistInstances, startingInstances subscriptions.WhistInstances) int {
	var expectedInstances int // The number of instances both in active and starting state

	availableInstances := GetAvailableInstances(imageID, activeInstances)

	// Loop over starting instances (status PRE_CONNECTION, only consider the ones with the current image
	for _, instance := range startingInstances {
		if instance.ImageID == graphql.String(imageID) {
			expectedInstances++
		}
	}

	return availableInstances + expectedInstances
}
