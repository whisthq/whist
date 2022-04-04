package utils // import "github.com/whisthq/whist/backend/services/utils"

import "github.com/google/uuid"

// This block contains some variables for directories we use in the host
// service. They're used in a lot of packages, so we put them in the least
// common denominator --- this package.

var (
	// Path to the "/whist" directory. Set as a variable instead of a constant
	// because its value might change if we are using ephemeral storage.
	WhistDir string = "/whist/"
	TempDir  string = WhistDir + "temp/"

	// WhistPrivateDir gets its own root path so that we avoid leaking our TLS
	// certificates to users even if they escape container access and can read
	// the entire `/whist/` directory.
	WhistPrivateDir string = "/whistprivate/"
)

// WhistEphemeralFSPath is the path where the ephemeral storage device will
// be mounted if it exists. If it does, all whist directories will be created
// on this path.
const WhistEphemeralFSPath = "/ephemeral/"

// Note: We use these values as placeholder UUIDs because they are obvious and immediate
// when parsing/searching through logs, and by using a non nil placeholder UUID we are
// able to detect the error when a UUID is nil.

// PlaceholderWarmupUUID returns the special uuid to use as a placholder during warmup.
func PlaceholderWarmupUUID() uuid.UUID {
	uuid, _ := uuid.Parse("11111111-1111-1111-1111-111111111111")
	return uuid
}

// PlaceholderTestUUID returns the special uuid to use as a placholder during tests.
func PlaceholderTestUUID() uuid.UUID {
	uuid, _ := uuid.Parse("22222222-2222-2222-2222-222222222222")
	return uuid
}
