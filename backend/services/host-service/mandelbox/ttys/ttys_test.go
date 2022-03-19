package ttys

import (
	"testing"
)

func TestAllocateAndFreeTTYIntegration(t *testing.T) {
	testTTY, err := Allocate()
	// Attempt to allocate TTY
	if err != nil {
		t.Fatalf("error initializing TTY with err: %v", err)
	}

	// check if TTY is within the range
	if !isInAllowedRange(uint8(testTTY)) {
		t.Fatalf("error range for test TTY as the result was not allowed: %v", testTTY)
	}

	if ttymap[testTTY] != inUse {
		t.Fatalf("error ttymap stored TTY %v with value: %v", testTTY, ttymap[testTTY])
	}

	// Free and check allocated mandelbox TTY
	Free(testTTY)

	if _, exists := ttymap[testTTY]; exists {
		t.Fatalf("error freeing TTY as mandelbox TTY still exists: %v", testTTY)
	}
}

func TestAllocateWithNoFreeTTY(t *testing.T) {
	// Fill out the entire map
	for allowedTTY := minAllowedTTY; allowedTTY < maxAllowedTTY; allowedTTY++ {
		ttymap[TTY(allowedTTY)] = inUse
	}

	// This will result in an error as there are no available entries
	testTTY, err := Allocate()
	if err == nil {
		t.Fatalf("error allocating TTY with none available. Expected err and tty: 0, got nil and tty: %v", testTTY)
	}

	// Free entire map
	for allowedTTY := minAllowedTTY; allowedTTY < maxAllowedTTY; allowedTTY++ {
		Free(TTY(allowedTTY))
	}

}

func TestFreeReservedTTY(t *testing.T) {
	// Add a reserved tty entry to ttymap
	ttymapLock.Lock()
	testTTY := randomTTYInAllowedRange()
	ttymap[testTTY] = reserved
	ttymapLock.Unlock()

	mapSizeBeforeFree := len(ttymap)

	// Attempt to remove reserved tty
	Free(testTTY)

	// Free should not have removed tty
	if mapSizeAfterFree := len(ttymap); mapSizeBeforeFree != mapSizeAfterFree {
		t.Fatalf("error ttymap expected size to be %v after free but is %v", mapSizeBeforeFree, mapSizeAfterFree)
	}

	// TTY should still exist
	if _, exists := ttymap[testTTY]; !exists {
		t.Fatalf("error test TTY does not exists after free: %v", testTTY)
	}
}

func TestFreeInUseTTY(t *testing.T) {
	// Add an inUse tty entry to ttymap
	ttymapLock.Lock()
	testTTY := randomTTYInAllowedRange()
	ttymap[testTTY] = inUse
	ttymapLock.Unlock()

	mapSizeBeforeFree := len(ttymap)

	// Attempt to remove inUse tty
	Free(testTTY)

	// Free should have removed only one tty
	if mapSizeAfterFree := len(ttymap); mapSizeBeforeFree-1 != mapSizeAfterFree {
		t.Fatalf("error ttymap expected size to be %v after free but is %v", mapSizeBeforeFree-1, mapSizeAfterFree)
	}

	// TTY should not exist
	if _, exists := ttymap[testTTY]; exists {
		t.Fatalf("error test TTY exists after free: %v", testTTY)
	}
}

func TestAllowedRangeMinInclusive(t *testing.T) {
	// MinAllowedTTY is inclusive and should be in the range
	if !isInAllowedRange(minAllowedTTY) {
		t.Fatalf("error checking in allowed range TTY as inclusive min was not allowed: %v", minAllowedTTY)
	}
}

func TestNotAllowedRangeMaxExclusive(t *testing.T) {
	// MaxAllowedTTY is exclusive and should not be in the range
	if isInAllowedRange(maxAllowedTTY) {
		t.Fatalf("error checking in allowed range TTY as exclusive max was allowed: %v", maxAllowedTTY)
	}
}

func TestNotAllowedRangeOutsideMinMax(t *testing.T) {
	// Anything greater than maxAllowedTTY should not be in the allowed range
	moreThanMaxAllowedTTY := uint8(maxAllowedTTY + 1)
	if isInAllowedRange(moreThanMaxAllowedTTY) {
		t.Fatalf("error checking in allowed range TTY as value more than exclusive max was allowed: %v", moreThanMaxAllowedTTY)
	}

	// MinAllowedTTY is the lowest bound and anything below should return false
	lessThanMinAllowedTTY := uint8(minAllowedTTY - 1)
	if isInAllowedRange(lessThanMinAllowedTTY) {
		t.Fatalf("error checking in allowed range TTY as value less than inclusive was allowed: %v", lessThanMinAllowedTTY)
	}
}

func TestAllowedRangeMaxMinusOne(t *testing.T) {
	// MaxAllowedTTY is exclusive meaning that one less is allowed
	oneLessThanMaxAllowedTTY := uint8(maxAllowedTTY - 1)
	if !isInAllowedRange(oneLessThanMaxAllowedTTY) {
		t.Fatalf("error checking in allowed range TTY as one less than exclusive max was not allowed: %v", oneLessThanMaxAllowedTTY)
	}
}

func TestRandomTTYInAllowedRange(t *testing.T) {
	// The random TTY should always be in the range
	randomTTY := uint8(randomTTYInAllowedRange())
	if !isInAllowedRange(randomTTY) {
		t.Fatalf("error getting random tty in allowed range TTY: %v", randomTTY)
	}
}
