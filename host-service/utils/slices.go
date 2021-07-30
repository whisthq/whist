package utils // import "github.com/fractal/fractal/host-service/utils"

// SliceContains returns true if the given slice contains val, and false otherwise.
func SliceContains(slice []interface{}, val interface{}) bool {
	for _, v := range slice {
		if v == val {
			return true
		}
	}
	return false
}
