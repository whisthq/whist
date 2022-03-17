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
	defaultInstanceBuffer = 1
	// desiredFreeMandelboxes is the number of free mandelboxes we always
	// want available in a region.
	desiredFreeMandelboxes = 2
)

// VCPUsPerMandelbox indicates the number of vCPUs allocated per mandelbox.
const VCPUsPerMandelbox = 4

var instanceTypeToGPUNum = map[string]int{
	"g4dn.xlarge":   1,
	"g4dn.2xlarge":  1,
	"g4dn.4xlarge":  1,
	"g4dn.8xlarge":  1,
	"g4dn.16xlarge": 1,
	"g4dn.12xlarge": 4,
}

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

// bundledRegions is a list of the enabled regions on the cloud providers.
// TODO: when adding multi-cloud support, figure out how to bundle regions
// for different cloud providers.
var BundledRegions = []string{"us-east-1", "us-east-2", "us-west-1", "us-west-2", "ca-central-1"}

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
		capacityMap[instanceType] = utils.Min(gpuNum*constants.MaxMandelboxesPerGPU, vcpuNum/VCPUsPerMandelbox)
	}
	return capacityMap
}
