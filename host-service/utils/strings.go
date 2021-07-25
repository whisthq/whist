package utils // import "github.com/fractal/fractal/host-service/utils"

import (
	"crypto/rand"
	"encoding/hex"
	"fmt"
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

// ColorRed returns the input string surrounded by the ANSI escape codes to
// color the text red. Text color is reset at the end of the returned string.
func ColorRed(s string) string {
	const (
		codeReset = "\033[0m"
		codeRed   = "\033[31m"
	)

	return Sprintf("%s%s%s", codeRed, s, codeReset)
}

// The following two functions exist so that we don't have to import `fmt` into
// any other packages (so we don't accidentally log something using `fmt`
// functions instead of using the `fractallogger` equivalents that send
// information to logz.io and Sentry).

// Sprintf creates a string from format string and args.
func Sprintf(format string, v ...interface{}) string {
	return fmt.Sprintf(format, v...)
}

// MakeError creates an error from format string and args.
func MakeError(format string, v ...interface{}) error {
	return fmt.Errorf(format, v...)
}
