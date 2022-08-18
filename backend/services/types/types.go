// Package types simply contains some useful types for the `mandelbox`
// package. We define this package separately so that we can safely pass these
// types around to other packages that `mandelbox` itself might depend
// on.
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
// sort of identifier for each mandelbox, and we need it before Docker
// gives us back the runtime Docker ID for the mandelbox.
type MandelboxID uuid.UUID

// A DockerID is provided by Docker at mandelbox creation time.
type DockerID string

// AppName is defined as its own type so, in the future, we can always easily
// enforce that it is part of a limited set of values.
type AppName string

// UserID is the id assigned to a user by the authentication provider (Auth0).
type UserID string

// SessionID can be a random hash or a timestamp, depending if it's a server-side
// session id, or a client-side session id. A server-side session id is created by
// the host service when starting a waiting mandelbox, and is represented by a random
// hash. A client-side session id is created by the frontend and is written when a
// mandelbox gets assigned to a user, it's represented by a Unix timestamp.
type SessionID string

// ConfigEncryptionToken is the token used to decrypt the user configurations.
type ConfigEncryptionToken string

// ClientAppAccessToken is a JWT created by the authentication provider and used to
// authenticate the user to Whist. It contains custom claims and metadata.
type ClientAppAccessToken string

// PrivateKey is an AES key used for connecting the protocol client to the protocol server
// running inside a mandelbox. Its purpose is to prevent other users from connecting to a
// mandelbox that is not assigned to them.
type PrivateKey string

// JSONData is a set of configurations sent by the frontend that can only be set before launching Chrome.
// Some examples are timezone, location, dark mode, DPI.
type JSONData string

// BrowserData is the collection of data imported from a local Chrome to the Chrome instance running inside the
// mandelbox. This can be a combination of cookies, extensions and local storage.
type BrowserData string

// Cookies is a json file containing user cookies.
type Cookies string

// Extensions is a comma-separated string containing user extensions.
type Extensions string

// LocalStorage is a json file containing user local storage.
type LocalStorage string

// ExtensionSettings is a json file containing user extension settings.
type ExtensionSettings string

// ExtensionState is a json file containing user extension state.
type ExtensionState string

// Custom type methods

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
