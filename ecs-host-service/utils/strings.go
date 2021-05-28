package utils // import "github.com/fractal/fractal/ecs-host-service/utils"

import (
	"crypto/rand"
	"encoding/hex"
	"strings"
)

// RandHex creates a hexadecimal string with the provided number of bytes of
// randomness. Therefore, the output string will have length 2 * numBytes.
func RandHex(numBytes uint8) string {
	b := make([]byte, numBytes)
	_, _ = rand.Read(b)
	return hex.EncodeToString(b)
}

// FindSubstringBetween gets the substring after the first occurrence of
// `start` and before the first occurrence of `end`.
func FindSubstringBetween(value string, start string, end string) string {
	posFirst := strings.Index(value, start)
	if posFirst == -1 {
		return ""
	}
	posLast := strings.Index(value, end)
	if posLast == -1 {
		return ""
	}
	posFirstAdjusted := posFirst + len(start)
	if posFirstAdjusted >= posLast {
		return ""
	}
	return value[posFirstAdjusted:posLast]
}
