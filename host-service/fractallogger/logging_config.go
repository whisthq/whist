package fractallogger // import "github.com/fractal/fractal/host-service/fractallogger"

import (
	"os"
	"strings"

	"github.com/fractal/fractal/host-service/metadata"
)

// usingProdLogging implements the logic for us to decide whether to use
// production-logging (i.e. with Sentry and logz.io configured).
var usingProdLogging func() bool = func(unmemoized func() bool) func() bool {
	// This nested function syntax is used to memoize the result of the first
	// call and cache the result for all future calls.

	isCached := false
	var cache bool

	return func() bool {
		if isCached {
			return cache
		}
		cache = unmemoized()
		isCached = true
		return cache
	}
}(func() bool {
	// Caching-agnostic logic goes here

	// Honor `USE_PROD_LOGGING` variable if it is set to a valid value. Else,
	// check that the app environment is dev, staging, or prod.
	strProd := strings.ToLower(os.Getenv("USE_PROD_LOGGING"))
	switch strProd {
	case "1", "yes", "true":
		return true
	case "0", "no", "false":
		return false
	default:
		return !metadata.IsLocalEnv()
	}
})
