package engine

import (
	"strings"
)

// findSubstringBetween gets the substring after the first occurrence of
// `start` and before the first occurrence of `end`.
func findSubstringBetween(value string, start string, end string) string {
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
