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

// SliceContainsString returns true if the given slice contains string val, and false otherwise.
func SliceContainsString(slice []string, val string) bool {
	for _, v := range slice {
		if v == val {
			return true
		}
	}
	return false
}

// SliceRemoveString deletes the first occurence of string val in s.
// https://github.com/golang/go/wiki/SliceTricks#delete-without-preserving-order
func SliceRemoveString(s []string, val string) []string {
	var i int
	var found bool
	for index := range s {
		if s[index] == val {
			i = index
			found = true
			break
		}
	}

	if found {
		s[i] = s[len(s)-1]
		s = s[:len(s)-1]
	}
	return s
}
