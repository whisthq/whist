package utils // import "github.com/whisthq/whist/backend/services/utils"

import "golang.org/x/exp/constraints"

// SliceContains returns true if the given slice contains val, and false otherwise.
func SliceContains(slice []interface{}, val interface{}) bool {
	for _, v := range slice {
		if v == val {
			return true
		}
	}
	return false
}

// SliceRemove deletes the first occurence of val in s.
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

// StringSliceContains returns true if the given string slice contains string val, and false otherwise.
func StringSliceContains(slice []string, val string) bool {
	for _, v := range slice {
		if v == val {
			return true
		}
	}
	return false
}

// PrintSlice is a helper function to print a slice as a string of comma separated values.
// The string is truncated to the first n elements in the slice, to improve readability.
func PrintSlice[T constraints.Ordered](slice []T, n int) string {
	if len(slice) < n {
		n = len(slice)
	}

	var message string
	for i, v := range slice[:n] {
		if i+1 == n {
			message += Sprintf("%v", v)
		} else {
			message += Sprintf("%v, ", v)
		}
	}
	return message
}
