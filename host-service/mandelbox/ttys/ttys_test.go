package ttys

import (
	"testing"
)

const (
	increment = 22
	decrement = 1
)

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
	moreThanMaxAllowedTTY := uint8(maxAllowedTTY + increment)
	if isInAllowedRange(moreThanMaxAllowedTTY) {
		t.Fatalf("error checking in allowed range TTY as value more than exclusive max was allowed: %v", moreThanMaxAllowedTTY)
	}

	// MinAllowedTTY is the lowest bound and anything below should return false
	lessThanMinAllowedTTY := uint8(minAllowedTTY - decrement)
	if isInAllowedRange(lessThanMinAllowedTTY) {
		t.Fatalf("error checking in allowed range TTY as value less than inclusive was allowed: %v", lessThanMinAllowedTTY)
	}
}

func TestAllowedRangeMaxMinusOne(t *testing.T) {
	// MaxAllowedTTY is exclusive meaning that one less is allowed
	oneLessThanMaxAllowedTTY := uint8(maxAllowedTTY - 1)
	if !isInAllowedRange(minAllowedTTY) {
		t.Fatalf("error checking in allowed range TTY as one less than exclusive max was not allowed: %v", oneLessThanMaxAllowedTTY)
	}
}

func TestRandomTTYInAllowRange(t *testing.T) {
	// The random TTY should always be in the range
	randomTTY := uint8(randomTTYInAllowedRange())
	if !isInAllowedRange(randomTTY) {
		t.Fatalf("error getting random tty in allowed range TTY: %v", randomTTY)
	}
}
