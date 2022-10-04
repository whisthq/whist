package scaling_algorithms

import (
	"time"

	"github.com/whisthq/whist/backend/services/constants"
	"github.com/whisthq/whist/backend/services/utils"
)

// Default configuration values. These values should be pulled from
// the config database, but we leave the default in case we fail to
// query the database.
var (
	// defaultInstanceBuffer is the number of instances with space to run
	// mandelboxes. This value is used when deciding how many instances to
	// scale up if we don't have enough capacity.
	defaultInstanceBuffer int = 1
	// desiredFreeMandelboxesPerRegion is the number of free mandelboxes we always
	// want available in a region. This value is set per-region and it represents
	// the free mandelboxes we want on each.
	desiredFreeMandelboxesPerRegion = map[string]int{
		"us-east-1": 2,
	}
)

// VCPUsPerMandelbox indicates the number of vCPUs allocated per mandelbox.
const VCPUsPerMandelbox = 4

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
		min := utils.Min(gpuNum*constants.MaxMandelboxesPerGPU, vcpuNum/VCPUsPerMandelbox)
		capacityMap[instanceType] = min
	}
	return capacityMap
}
