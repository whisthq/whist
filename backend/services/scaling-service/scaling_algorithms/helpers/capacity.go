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

<<<<<<< HEAD
// GetExpectedInstances is a helper function to get the number of instances that are
// active and have space to run mandelboxes, summed with the number of instances that
// are on a starting state and will soon be active.
func GetExpectedInstances(imageID string, activeInstances subscriptions.WhistInstances, startingInstances subscriptions.WhistInstances) int {
	var expectedInstances int // The number of instances both in active and starting state
=======
// ComputeExpectedCapacity is a helper function to compute the expected capacity on a given region-image
// pair. This function takes into account the number of instances which can serve, as well as the number
// of mandelboxes they can run. We define expected capacity as the number of instances in active state
// which have space to run mandelboxes added to the number of starting instances (PRE_CONNECTION status). This function returns the expected capacity
// and the number of mandelboxes which can be started with the instance buffer.
func ComputeExpectedCapacity(imageID string, region string, activeInstances subscriptions.WhistInstances, startingInstances subscriptions.WhistInstances) (int, int) {
	var (
		expectedCapacity  int // The number of instances both in active and starting state
		mandelboxCapacity int // The total number of mandelboxes that can be started
	)
>>>>>>> a6c575473 (Clarify comments)

	availableInstances := GetAvailableInstances(imageID, activeInstances)

	// Loop over starting instances (status PRE_CONNECTION, only consider the ones with the current image
	for _, instance := range startingInstances {
		if instance.ImageID == graphql.String(imageID) {
			expectedInstances++
		}
	}

	return availableInstances + expectedInstances
}
