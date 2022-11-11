package scaling

import (
	"math"
	"time"

	"github.com/whisthq/whist/backend/elegant_monolith/internal"
)

const (
	// MaxMandelboxesPerGPU represents the maximum of mandelboxes we can assign to
	// each GPU and still maintain acceptable performance. Note that we are
	// assuming that all GPUs on a mandelbox are uniform (and that we are using
	// only one instance type).
	MaxMandelboxesPerGPU = 3

	// VCPUsPerMandelbox indicates the number of vCPUs allocated per mandelbox.
	VCPUsPerMandelbox = 4
)

// A map containing how many GPUs each instance type has.
var instanceTypeToGPUNum = map[string]int{
	"g4dn.xlarge":   1,
	"g4dn.2xlarge":  1,
	"g4dn.4xlarge":  1,
	"g4dn.8xlarge":  1,
	"g4dn.16xlarge": 1,
	"g4dn.12xlarge": 4,
}

// A map containing how many VCPUs each instance type has.
var instanceTypeToVCPUNum = map[string]int{
	"g4dn.xlarge":   4,
	"g4dn.2xlarge":  8,
	"g4dn.4xlarge":  16,
	"g4dn.8xlarge":  32,
	"g4dn.16xlarge": 64,
	"g4dn.12xlarge": 48,
}

// instanceCapacity is a mapping of the mandelbox capacity each type of instance has.
var instanceCapacity = generateInstanceCapacityMap(instanceTypeToGPUNum, instanceTypeToVCPUNum)

var (
	// maxWaitTimeReady is the max time we whould wait for instances to be ready.
	maxWaitTimeReady = 5 * time.Minute
	// maxWaitTimeTerminated is the max time we whould wait for instances to be terminated.
	maxWaitTimeTerminated = 5 * time.Minute
)

// generateInstanceCapacityMap uses the global instanceTypeToGPUNum and instanceTypeToVCPUNum maps
// to generate the maximum mandelbox capacity for each instance type in the intersection
// of their keys.
func generateInstanceCapacityMap(instanceToGPUMap, instanceToVCPUMap map[string]int) map[string]int {
	// Initialize the instance capacity map
	capacityMap := map[string]int{}
	for instanceType, gpuNum := range instanceToGPUMap {
		// Only populate for instances that are in both maps
		vcpuNum, ok := instanceToVCPUMap[instanceType]
		if !ok {
			continue
		}
		min := math.Min(float64(gpuNum*MaxMandelboxesPerGPU), float64(vcpuNum/VCPUsPerMandelbox))
		capacityMap[instanceType] = int(min)
	}
	return capacityMap
}

// ComputeRealMandelboxCapacity is a helper function to compute the real mandelbox capacity.
// Real capcity is the number of free mandelboxes available on instances with active status.
func computeRealMandelboxCapacity(imageID string, activeInstances []internal.Instance) int {
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
func computeExpectedMandelboxCapacity(imageID string, activeInstances []internal.Instance, startingInstances []internal.Instance) int {
	var (
		realMandelboxCapacity     int
		expectedMandelboxCapacity int
	)

	// Get the capacity from active instances
	realMandelboxCapacity = computeRealMandelboxCapacity(imageID, activeInstances)

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
func computeInstancesToScale(desiredMandelboxes int, currentCapacity int, instanceCapacity int) int {
	// This will never happen realistically, but we should try to handle all cases.
	if instanceCapacity <= 0 {
		return 0
	}

	desiredCapacity := desiredMandelboxes - currentCapacity
	instancesToScale := math.Ceil(float64(desiredCapacity) / float64(instanceCapacity))

	return int(instancesToScale)
}
