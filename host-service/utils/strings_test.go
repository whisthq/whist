package utils

import (
	"testing"
)

// TestRandHexLen will check if the output string length matches the func comment
func TestRandHexLen(t *testing.T) {
	numBytes := uint8(8)
	
	res := RandHex(numBytes)	
	if int(numBytes) * 2 != len(res) {
		t.Fatalf("error getting randhex: %s. Expected length %d, got %d", res, int(numBytes) * 2, len(res))
	}
}

// TestRandHexLenZero will check if the output string will be the empty string
func TestRandHexLenZero(t *testing.T) {
	numBytes := uint8(0)
	
	res := RandHex(numBytes)	
	if len(res) != 0  {
		t.Fatalf("error getting randhex: %s. Expected length %d, got %d", res, 0, len(res))
	}
}

// TestFindSubstringBetween will get the substring
func TestFindSubstringBetween(t *testing.T) {
	start := "ice"
	substring := " test substring "
	end := "age"
	value := "this is filler" + start + substring + end + start + "more filler" + end

	// FindSubstringBetween will return the substring
	res := FindSubstringBetween(value, start, end)

	if res != substring {
		t.Fatalf("error finding substring between `%s`. Expected `%s`, got `%s`", value, substring, res)
	}

}


// TestFindSubstringBetweenMissingStart will get the substring
func TestFindSubstringBetweenMissingStart(t *testing.T) {
	start := "ice"
	substring := " test substring "
	end := "age"
	value := "this is filler" + substring + end + "more filler" + end

	// FindSubstringBetween will find the index of the start is -1 and return an empty string
	res := FindSubstringBetween(value, start, end)

	if res != "" {
		t.Fatalf("error finding substring between `%s` with start not included in value. Expected `%s`, got `%s`", value, "", res)
	}
}

// TestFindSubstringBetweenMissingEnd will get the substring
func TestFindSubstringBetweenMissingEnd(t *testing.T) {
	start := "ice"
	substring := " test substring "
	end := "age"
	value := "this is filler" + substring + start + "more filler" + start

	// FindSubstringBetween will find the index of the end is -1 and return an empty string
	res := FindSubstringBetween(value, start, end)

	if res != "" {
		t.Fatalf("error finding substring between `%s` with end not included in value. Expected `%s`, got `%s`", value, "", res)
	}
}

// TestFindSubstringBetweenStartFoundAfterEnd will get the substring
func TestFindSubstringBetweenStartFoundAfterEnd(t *testing.T) {
	start := "ice"
	substring := " test substring "
	end := "age"
	value := "this is filler" + substring + end + "more filler" + start + end

	// FindSubstringBetween will find the index of the start is greater than the index of the end  and return an empty string
	res := FindSubstringBetween(value, start, end)

	if res != "" {
		t.Fatalf("error finding substring between `%s` with start found after end in value. Expected `%s`, got `%s`", value, "", res)
	}
}

// TestColorRed will get the colour red
func TestColorRed(t *testing.T) {
	s := "testing colour"
	color := ColorRed(s)
	if expectedColor := "\033[31m"+ s + "\033[0m" ; color != expectedColor {
		t.Fatalf("error getting color red. Expected %v, got %v", string(expectedColor), color)
	}
}


// TestMakeError will confirm the error is properly formatted
func TestMakeError(t *testing.T) {
	errMsg := "MakeError must return an error identical to this 1."
	formatMsg := "%s must return an error identical to this %d."

	// MakeError will generate a valid error with the provided format string and params
	err := MakeError(formatMsg, "MakeError", 1)

	if err == nil {
		t.Fatal("error making an error. Expected err, got nil")
	}

	if err.Error() != errMsg {
		t.Fatalf("error making an error. Expected %s, got %s", errMsg, err.Error())
	}
}
