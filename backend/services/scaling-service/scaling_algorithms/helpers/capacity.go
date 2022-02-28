package helpers

import (
	"github.com/hasura/go-graphql-client"
	"github.com/whisthq/whist/backend/services/subscriptions"
)

<<<<<<< HEAD
// GetAvailableInstances is a helper function to get the number of instances that are
// active and have space to run mandelboxes.
func GetAvailableInstances(imageID string, activeInstances subscriptions.WhistInstances) int {
	var availableInstances int // The number of instances which are active and have space
=======
// ComputeRealCapacity is a helper function to compute the actual capacity on a given region-image
// pair. This function takes into account the number of instances which can serve, as well as the
// number of mandelboxes they can run. We define real capacity as the number of instances which have
// space to run mandelboxes.
func ComputeRealCapacity(imageID string, activeInstances subscriptions.WhistInstances) int {
	var realCapacity int // The number of instances which are active and have space
>>>>>>> 18f0092a3 (Remove mandelbox capacity)

	// Loop over active instances (status ACTIVE), only consider the ones with the current image
	for _, instance := range activeInstances {
		if instance.ImageID == graphql.String(imageID) &&
			instance.RemainingCapacity > 0 {
<<<<<<< HEAD
			availableInstances++
		}
	}

	return availableInstances
=======
			realCapacity++
		}
	}

	return realCapacity
>>>>>>> 18f0092a3 (Remove mandelbox capacity)
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
<<<<<<< HEAD
// which have space to run mandelboxes added to the number of starting instances (PRE_CONNECTION status). This function returns the expected capacity
// and the number of mandelboxes which can be started with the instance buffer.
func ComputeExpectedCapacity(imageID string, region string, activeInstances subscriptions.WhistInstances, startingInstances subscriptions.WhistInstances) (int, int) {
	var (
		expectedCapacity  int // The number of instances both in active and starting state
		mandelboxCapacity int // The total number of mandelboxes that can be started
	)
>>>>>>> a6c575473 (Clarify comments)

	availableInstances := GetAvailableInstances(imageID, activeInstances)
=======
// which have space to run mandelboxes added to the number of starting instances (PRE_CONNECTION status).
func ComputeExpectedCapacity(imageID string, activeInstances subscriptions.WhistInstances, startingInstances subscriptions.WhistInstances) int {
	var expectedCapacity int // The number of instances both in active and starting state

	realCapacity := ComputeRealCapacity(imageID, activeInstances)
>>>>>>> 18f0092a3 (Remove mandelbox capacity)

	// Loop over starting instances (status PRE_CONNECTION, only consider the ones with the current image
	for _, instance := range startingInstances {
		if instance.ImageID == graphql.String(imageID) {
			expectedInstances++
		}
	}

<<<<<<< HEAD
	return availableInstances + expectedInstances
=======
	return realCapacity + expectedCapacity
>>>>>>> 18f0092a3 (Remove mandelbox capacity)
}
