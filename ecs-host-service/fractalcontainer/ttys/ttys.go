package ttys // import "github.com/fractal/fractal/ecs-host-service/fractalcontainer/ttys"

import (
	"math/rand"
	"sync"

	logger "github.com/fractal/fractal/ecs-host-service/fractallogger"
)

// TTY is defined as a type so that the implementation can change without the
// package API changing. Also, there isn't a fundamental restriction for us to
// stay below 256, but this should be plenty (we can always change it later).
type TTY uint8

type ttyStatus byte

const (
	// We need to allocate the first 10 for the system.
	minAllowedTTY = 10 // inclusive
	maxAllowedTTY = 64 //exclusive

	reserved ttyStatus = iota
	inUse
)

func init() {
	logger.Infof("TESTING IF THIS PACKAGE IS GETTING INITIALIZED TWICEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE. %v", &ttymap)
}

// If the key exists in this map, that TTY is either `reserved` or `inUse`.
var ttymap map[TTY]ttyStatus = make(map[TTY]ttyStatus)

// Lock to protect `ttymap`
var ttymapLock = new(sync.Mutex)

// Allocate allocates a free TTY.
func Allocate() (TTY, error) {
	ttymapLock.Lock()
	defer ttymapLock.Unlock()

	tty := randomTTYInAllowedRange()
	numTries := 0

	for v, exists := ttymap[tty]; exists; tty = randomTTYInAllowedRange() {
		logger.Infof("TTY: tried tty %v but it exists with value %v", tty, v)
		numTries++
		if numTries >= 100 {
			return TTY(0), logger.MakeError("Tried %v times to allocate a TTY for container. Breaking out to avoid spinning for too long. Number of allocated TTYs: %v", numTries, len(ttymap))
		}
	}

	// Mark it as allocated and return
	ttymap[tty] = inUse
	logger.Infof("TTY: allocated tty %v", tty)
	return tty, nil
}

// Free marks the given TTY as free, if it is not reserved.
func Free(tty TTY) {
	ttymapLock.Lock()
	defer ttymapLock.Unlock()

	v, _ := ttymap[tty]
	if v != reserved {
		delete(ttymap, tty)
		logger.Infof("TTY: freed tty %v", tty)
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
