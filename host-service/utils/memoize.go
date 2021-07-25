package utils // import "github.com/fractal/fractal/host-service/utils"

// MemoizeStringWithError lets us memoize a function of type (func() (string,
// error)) into another function of the same type.  The function returned from
// MemoizeStringWithError does not cache the results from a failed call (i.e.
// non-nil error). It only caches the return value from a successful call to
// the argument function `f`. However, once the argument function `f` is called
// successfully once, all future calls to the returned function will return the
// cached result of the successful call, and a nil error.
func MemoizeStringWithError(f func() (string, error)) func() (string, error) {
	var cached bool = false
	var cachedResult string
	return func() (string, error) {
		if cached {
			return cachedResult, nil
		}
		result, err := f()
		if err != nil {
			return result, err
		}
		cachedResult = result
		cached = true
		return cachedResult, nil
	}
}
