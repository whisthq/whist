package ttys // import "github.com/fractal/fractal/host-service/mandelbox/ttys"

import (
	"math/rand"
	"sync"

	logger "github.com/fractal/fractal/host-service/fractallogger"
	"github.com/fractal/fractal/host-service/utils"
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

// If the key exists in this map, that TTY is either `reserved` or `inUse`.
var ttymap = make(map[TTY]ttyStatus)

// Lock to protect `ttymap`
var ttymapLock = new(sync.Mutex)

// Allocate allocates a free TTY.
func Allocate() (TTY, error) {
	ttymapLock.Lock()
	defer ttymapLock.Unlock()

	var tty TTY
	maxTries := 100
	for numTries := 0; numTries < maxTries; numTries++ {
		tty = randomTTYInAllowedRange()
		if _, exists := ttymap[tty]; !exists {
			break
		}
	}
	if tty == TTY(0) {
		return TTY(0), utils.MakeError("Tried %v times to allocate a TTY. Breaking out to avoid spinning for too long. Number of currently-allocated TTYs: %v", maxTries, len(ttymap))
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
