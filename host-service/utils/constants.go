package utils // import "github.com/fractal/fractal/host-service/utils"

import "github.com/google/uuid"

// This block contains some constants for directories we use in the host
// service. They're used in a lot of packages, so we put them in the least
// common denominator --- this package.
const (
	FractalDir        string = "/fractal/"
	TempDir           string = FractalDir + "temp/"
	FractalPrivateDir string = "/fractalprivate/"
)

// PlaceholderUUID returns the special uuid to use as a placholder during warmup/tests.
func PlaceholderUUID() uuid.UUID {
	uuid, _ := uuid.Parse("11111111-1111-1111-1111-111111111111")
	return uuid
}
