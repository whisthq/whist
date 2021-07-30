package utils // import "github.com/fractal/fractal/host-service/utils"

// This block contains some constants for directories we use in the host
// service. They're used in a lot of packages, so we put them in the least
// common denominator --- this package.
const (
	FractalDir        string = "/fractal/"
	TempDir           string = FractalDir + "temp/"
	FractalPrivateDir string = "/fractalprivate/"
)
