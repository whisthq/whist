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

// SliceRemove deletes the first occurence of string val in s.
// This represents a small memory leak.
// https://github.com/golang/go/wiki/SliceTricks#delete-without-preserving-order
func SliceRemove(s []interface{}, val interface{}) []interface{} {
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
