// Copyright (c) 2021-2022 Whist Technologies, Inc.

/*
Package helpers contains utility functions that can be shared between actions and
algorithms that handle sanitizing, parsing and computing values. It also includes
some functions that make it easier to test the scaling service locally.
*/

package helpers

import (
	"math"

	"github.com/whisthq/whist/backend/services/subscriptions"
)

// ComputeRealMandelboxCapacity is a helper function to compute the real mandelbox capacity.
// Real capcity is the number of free mandelboxes available on instances with active status.
func ComputeRealMandelboxCapacity(imageID string, activeInstances []subscriptions.Instance) int {
	var realMandelboxCapacity int

	// Loop over active instances (status ACTIVE), only consider the ones with the current image
	for _, instance := range activeInstances {
		if instance.ImageID == imageID {
			realMandelboxCapacity += instance.RemainingCapacity
		}
	}

	return realMandelboxCapacity
}

// ComputeExpectedMandelboxCapacity is a helper function to compute the expected mandelbox capacity.
// Expected capacity is the number of free mandelboxes available on instances with active status and
// in starting instances.
func ComputeExpectedMandelboxCapacity(imageID string, activeInstances []subscriptions.Instance, startingInstances []subscriptions.Instance) int {
	var (
		realMandelboxCapacity     int
		expectedMandelboxCapacity int
	)

	// Get the capacity from active instances
	realMandelboxCapacity = ComputeRealMandelboxCapacity(imageID, activeInstances)

	// Loop over starting instances (status PRE_CONNECTION), only consider the ones with the current image
	for _, instance := range startingInstances {
		if instance.ImageID == imageID {
			expectedMandelboxCapacity += instance.RemainingCapacity
		}
	}

	expectedMandelboxCapacity += realMandelboxCapacity
	return expectedMandelboxCapacity
}

// ComputeInstancesToScale computes the number of instances we want to scale, according to the current number of desired mandelboxes.
// desiredMandelboxes: the number of free mandelboxes we want at all times. Set in the config db.
// currentCapacity: the current free mandelboxes in the database. Obtained by querying database and computing capacity.
// instanceCapacity: the mandelbox capacity for the specific instance type in use (e.g. a "g4dn.2xlarge" instance can run 2 mandelboxes).
func ComputeInstancesToScale(desiredMandelboxes int, currentCapacity int, instanceCapacity int) int {
	if instanceCapacity <= 0 {
		return 1
	}

	desiredCapacity := desiredMandelboxes - currentCapacity
	instancesToScale := math.Ceil(float64(desiredCapacity) / float64(instanceCapacity))

	return int(instancesToScale)
}
