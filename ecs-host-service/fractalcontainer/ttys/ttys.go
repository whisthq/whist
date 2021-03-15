package ttys

import (
	"math/rand"
	"sync"

	logger "github.com/fractal/fractal/ecs-host-service/fractallogger"
)

// There isn't a fundamental restriction for us to stay below 256, but this should be plenty.
type TTY uint8

type ttyStatus byte

const (
	// We need to allocate the first 10 for the system.
	minAllowedTTY = 10  // inclusive
	maxAllowedTTY = 256 //exclusive

	reserved ttyStatus = iota
	inUse
)

// If the key exists in this map, that TTY is either `reserved` or `inUse`.
var ttymap map[TTY]ttyStatus

// Lock to protect `ttymap`
var ttymapLock = new(sync.Mutex)

func Allocate() (TTY, error) {
	ttymapLock.Lock()
	defer ttymapLock.Unlock()

	tty := randomTTYInAllowedRange()
	numTries := 0

	for _, exists := ttymap[tty]; exists; tty = randomTTYInAllowedRange() {
		numTries++
		if numTries >= 100 {
			return TTY(0), logger.MakeError("Tried %v times to allocate a TTY for container. Breaking out to avoid spinning for too long.", numTries)
		}
	}

	// Mark it as allocated and return
	ttymap[tty] = inUse
	return tty, nil
}

func Free(tty TTY) {
	ttymapLock.Lock()
	defer ttymapLock.Unlock()

	v, _ := ttymap[tty]
	if v != reserved {
		delete(ttymap, tty)
	}
}

// Helper function to generate random tty values in the allowed range
func randomTTYInAllowedRange() TTY {
	return TTY(minAllowedTTY + rand.Intn(maxAllowedTTY-minAllowedTTY))
}

// Helper function to determine whether a given tty is in the allowed range, or not
func isInAllowedRange(p uint16) bool {
	return p >= minAllowedTTY && p < maxAllowedTTY
}
