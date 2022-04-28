// Package types simply contains some useful types for the `mandelbox`
// package. We define this package separately so that we can safely pass these
// types around to other packages that `mandelbox` itself might depend
// on.
//
package types // import "github.com/whisthq/whist/backend/services/types"

import (
	"encoding/json"
	"strings"

	"github.com/google/uuid"
	"github.com/whisthq/whist/backend/services/utils"
)

// We define special types for the following string types for all the benefits
// of type safety, including making sure we never switch Docker and Whist
// IDs, for instance.

// A MandelboxID is a random string created for each mandelbox. We need some
// sort of identifier for each mandelbox, and we need it _before_ Docker
// gives us back the runtime Docker ID for the mandelbox.
type MandelboxID uuid.UUID

// A DockerID is provided by Docker at mandelbox creation time.
type DockerID string

// AppName is defined as its own type so, in the future, we can always easily
// enforce that it is part of a limited set of values.
type AppName string

// UserID is defined as its own type as well so the compiler can check argument
// orders, etc. more effectively.
type UserID string

// SessionID is defined as its own type for similar reasons.
type SessionID string

// ConfigEncryptionToken is defined as its own type for similar reasons.
type ConfigEncryptionToken string

// ClientAppAccessToken is defined as its own type for similar reasons.
type ClientAppAccessToken string

// AESKey is defined as its own type for similar reasons.
type AESKey string

// JSONData is defined as its own type for similar reasons.
type JSONData string

// Cookies is defined as its own type for similar reasons.
type Cookies string

// Bookmarks is defined as its own type for similar reasons.
type Bookmarks string

// Extensions is defined as its own type for similar reasons.
type Extensions string

// Preferences is defined as its own type for similar reasons.
type Preferences string

// LocalStorage is defined as its own type for similar reasons.
type LocalStorage string

// String is a utility function to return the string representation of a MandelboxID.
func (mandelboxID MandelboxID) String() string {
	return uuid.UUID(mandelboxID).String()
}

// MarshalJSON is a utility function to properly marshal a mandelboxID into a proper JSON representation
func (mandelboxID MandelboxID) MarshalJSON() ([]byte, error) {
	u := uuid.UUID(mandelboxID)
	UUID, err := uuid.Parse(u.String())

	if err != nil {
		return nil, utils.MakeError("Received invalid UUID when marshaling")
	}

	bytes, err := json.Marshal(UUID.String())

	if err != nil {
		return nil, utils.MakeError("Error marshaling UUID")
	}

	return bytes, nil
}

// UnmarshalJSON is a utility function to properly unmarshal JSON into a type MandelboxID
func (mandelboxID *MandelboxID) UnmarshalJSON(b []byte) error {
	s := strings.Trim(string(b), "\"")
	UUID, err := uuid.Parse(s)

	if err != nil {
		return utils.MakeError("Error parsing UUID")
	}

	*mandelboxID = MandelboxID(UUID)
	return nil
}
