package scaling_algorithms

import "time"

const (
	// DEFAULT_INSTANCE_BUFFER is the number of instances that should always
	// be running on each region.
	DEFAULT_INSTANCE_BUFFER = 1
	// MINIMUM_MANDELBOX_CAPACITY is the minimum number of mandelboxes that
	// we should always be able to run. This limit is used to make scaling decisions.
	// It acts as a warning for when we have almost all of our instances full.
	MINIMUM_MANDELBOX_CAPACITY = 2
)

// instanceCapacity is a mapping of the mandelbox capacity each type of instance has.
var instanceCapacity = map[string]int{
	"g4dn.2xlarge": 3,
}

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
