package global

import (
	"sync"
	"time"

	"github.com/whisthq/whist/backend/services/constants"
	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/subscriptions"
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
	// desiredFreeMandelboxesPerRegion is the number of free mandelboxes we always
	// want available in a region. This value is set per-region and it represents
	// the free mandelboxes we want on each.
	desiredFreeMandelboxesPerRegion = map[string]int{
		"us-east-1": 2,
	}
	// clientAppVersion represents the current version of the client app
	// (e.g. "2.6.13").
	clientAppVersion *subscriptions.ClientAppVersion
	// A lock to update the version once it gets switched on the database
	versionLock = &sync.Mutex{}
)

const (
	CLIENT_COMMIT_HASH_DEV_OVERRIDE = "local_dev"

	// These are all the possible reasons we would fail to find an instance for a user
	// and return a 503 error

	// Instance was found but the client app is out of date
	COMMIT_HASH_MISMATCH = "COMMIT_HASH_MISMATCH"

	// No instance was found e.g. a capacity issue
	NO_INSTANCE_AVAILABLE = "NO_INSTANCE_AVAILABLE"

	// The requested region(s) have not been enabled
	REGION_NOT_ENABLED = "REGION_NOT_ENABLED"

	// User is already connected to a mandelbox, possibly on another device
	USER_ALREADY_ACTIVE = "USER_ALREADY_ACTIVE"
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

// GetEnabledRegions returns a list of regions where the backend resources required
// to run Whist exist, according to the current environment.
func GetEnabledRegions() []string {
	var enabledRedions []string
	switch metadata.GetAppEnvironmentLowercase() {
	case string(metadata.EnvDev):
		enabledRedions = []string{
			"us-east-1",
		}
	case string(metadata.EnvStaging):
		enabledRedions = []string{
			"us-east-1",
		}
	case string(metadata.EnvProd):
		enabledRedions = []string{
			"us-east-1",
			"us-east-2",
			"us-west-1",
			"us-west-2",
			"ca-central-1",
			"eu-west-1",
			"eu-west-2",
			"eu-west-3",
			"eu-central-1",
			"ap-south-1",
			"ap-southeast-1",
			"ap-southeast-2",
		}
	default:
		enabledRedions = []string{
			"us-east-1",
		}
	}

	return enabledRedions
}

// getFrontendVersion returns the current version of the frontend, which is initially
// populated from the config database. This value is used by the scaling algorithm to
// determine if the incoming requests come from an outdated frontend, and is part of
// common configuration values shared by the scaling algorithms. Its necessary to grab
// a lock because multiple scaling algorithms read and update it.
func getFrontendVersion() string {
	versionLock.Lock()
	defer versionLock.Unlock()

	var version string

	if clientAppVersion == nil {
		return ""
	}

	switch metadata.GetAppEnvironment() {
	case metadata.EnvDev:
		version = utils.Sprintf("%v.%v.%v-dev-rc.%v", clientAppVersion.Major, clientAppVersion.Minor, clientAppVersion.Micro, clientAppVersion.DevRC)
	case metadata.EnvStaging:
		version = utils.Sprintf("%v.%v.%v-staging-rc.%v", clientAppVersion.Major, clientAppVersion.Minor, clientAppVersion.Micro, clientAppVersion.StagingRC)
	default:
		version = utils.Sprintf("%v.%v.%v", clientAppVersion.Major, clientAppVersion.Minor, clientAppVersion.Micro)
	}

	return version
}

// setFrontendVersion sets the frontend version we track locally. It does not update the value in the config database,
// only the configuration variable defined in this file shared between scaling algorithms. This function is only used
// when starting the scaling algorithm, and when the CI has updated the config database. Its necessary to grab a lock
// because multiple scaling algorithms read and update it.
func setFrontendVersion(newVersion subscriptions.ClientAppVersion) {
	versionLock.Lock()
	defer versionLock.Unlock()

	clientAppVersion = &newVersion
}
