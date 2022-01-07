package scaling_algorithms

// DEFAULT_INSTANCE_BUFFER is the number of instances that should always
// be running on each region.
const (
	DEFAULT_INSTANCE_BUFFER = 1 // number of instances that should always be running on each region.
)

// instanceCapacity is a mapping of the mandelbox capacity each type of instance has.
var instanceCapacity = map[string]int{
	"g4dn.2xlarge": 3,
}

var bundledRegions = []string{"us-east-1"}
