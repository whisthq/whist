package scaling_algorithms

import "time"

const (
	// DEFAULT_INSTANCE_BUFFER is the number of instances with space to run
	// mandelboxes. This value is used when deciding how many instances to
	// scale up if we don't have enough capacity.
	DEFAULT_INSTANCE_BUFFER = 1
	// DESIRED_FREE_MANDELBOXES is the number of free mandelboxes we always
	// want available in a region. TODO: Once we can connect to the hasura config server
	// fill in this value with the one in the database.
	DESIRED_FREE_MANDELBOXES = 2
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
