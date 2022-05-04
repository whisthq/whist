package helpers

import (
	"regexp"

	"github.com/whisthq/whist/backend/services/utils"
)

// SanitizeEmail tries to match an email to a general email regex
// and if it fails, returns an empty string. This is done because
// the email can be spoofed from the frontend.
func SanitizeEmail(email string) (string, error) {
	emailRegex := `(^[a-zA-Z0-9_.+-]+@[a-zA-Z0-9-]+\.[a-zA-Z0-9-.]+$)`

	match, err := regexp.Match(emailRegex, []byte(email))
	if err != nil {
		return "", utils.MakeError("error sanitizing user email. Err: %v", err)
	}

	if match {
		return email, nil
	} else {
		return "", nil
	}
}
