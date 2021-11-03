package utils // import "github.com/fractal/fractal/host-service/utils"

import "github.com/google/uuid" // This block contains some constants for directories we use in the host
// service. They're used in a lot of packages, so we put them in the least
// common denominator --- this package.
const (
	FractalDir        string = "/fractal/"
	TempDir           string = FractalDir + "temp/"
	FractalPrivateDir string = "/fractalprivate/"
)

// NilUUID is the special nil uuid with al bits set to zero. We use it as a "dummy" uuid.
var NilUUID = uuid.MustParse("00000000-0000-0000-0000-000000000000")
