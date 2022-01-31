package scaling_algorithms

// DEFAULT_INSTANCE_BUFFER is the number of instances that should always
// be running on each region.
const DEFAULT_INSTANCE_BUFFER = 1

// instanceCapacity is a mapping of the mandelbox capacity each type of instance has.
var instanceCapacity = map[string]int{
	"g4dn.2xlarge": 3,
}

// bundledRegions is a list of the enabled regions on the cloud providers.
// TODO: when adding multi-cloud support, figure out how to bundle regions
// for different cloud providers.
var BundledRegions = []string{"us-east-1", "us-east-2", "us-west-1", "us-west-2", "ca-central-1"}
