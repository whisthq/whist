package helpers

import (
	"regexp"

	"github.com/whisthq/whist/backend/services/utils"
)

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
