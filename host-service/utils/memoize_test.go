package utils

import (
	"testing"
)

// TestMemoizeStringWithError will test if the returned function functions properly
func TestMemoizeStringWithError(t *testing.T) {
	var visitedCount int
	expectedCached := "test"
	testFunc := func () (string, error) {
		// Increment count each time function is called
		visitedCount += 1
		return expectedCached, nil
	}

	newFunc := MemoizeStringWithError(testFunc)

	// newFunc will call testFunc once and cache the result
	if res, err := newFunc(); res != expectedCached || err != nil {
		t.Fatalf("error memoizing string with error. Expected `%s` and nil, got `%s` and %v", expectedCached, res, err)
	}

	// The second time will use the cached result
	if res, err := newFunc(); res != expectedCached || err != nil {
		t.Fatalf("error memoizing string with error. Expected `%s` and nil, got `%s` and %v", expectedCached, res, err)
	}

	if visitedCount != 1 {
		t.Fatalf("error memoizing string with error. Expected test func to be visited %d times, got %d", 1, visitedCount)
	}
}

// TestMemoizeStringWithError will test if the returned function functions properly
func TestMemoizeStringWithErrorAlwaysError(t *testing.T) {
	var visitedCount int
	expectedCached := "test"
	testFunc := func () (string, error) {
		// Increment count each time function is called
		visitedCount += 1
		return expectedCached, MakeError("test error")
	}

	newFunc := MemoizeStringWithError(testFunc)

	// newFunc will call testFunc once and cache the result
	if res, err := newFunc(); res != expectedCached || err == nil {
		t.Fatalf("error memoizing string with error. Expected `%s` and err, got `%s` and %v", expectedCached, res, err)
	}

	// The second time will use the cached result
	if res, err := newFunc(); res != expectedCached || err == nil {
		t.Fatalf("error memoizing string with error. Expected `%s` and err, got `%s` and %v", expectedCached, res, err)
	}

	if visitedCount != 2 {
		t.Fatalf("error memoizing string with error. Expected test func to be visited %d times, got %d", 2, visitedCount)
	}
}