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

type (
	// A MandelboxID is a random string created for each mandelbox. We need some
	// sort of identifier for each mandelbox, and we need it before Docker
	// gives us back the runtime Docker ID for the mandelbox.
	MandelboxID uuid.UUID

	// A DockerID is provided by Docker at mandelbox creation time.
	DockerID string

	// AppName is defined as its own type so, in the future, we can always easily
	// enforce that it is part of a limited set of values.
	AppName string

	// UserID is the id assigned to a user by the authentication provider (Auth0).
	UserID string

	// SessionID can be a random hash or a timestamp, depending if it's a server-side
	// session id, or a client-side session id. A server-side session id is created by
	// the host service when starting a waiting mandelbox, and is represented by a random
	// hash. A client-side session id is created by the frontend and is written when a
	// mandelbox gets assigned to a user, it's represented by a Unix timestamp.
	SessionID string

	// ClientAppAccessToken is a JWT created by the authentication provider and used to
	// authenticate the user to Whist. It contains custom claims and metadata.
	ClientAppAccessToken string

	// PrivateKey is an AES key used for connecting the protocol client to the protocol server
	// running inside a mandelbox. Its purpose is to prevent other users from connecting to a
	// mandelbox that is not assigned to them.
	PrivateKey string

	// JSONData is a set of configurations sent by the frontend that can only be set before launching Chrome.
	// Some examples are timezone, location, dark mode, DPI.
	JSONData string

	// Cookies is a json file containing user cookies.
	Cookies string

	// Extensions is a comma-separated string containing user extensions.
	Extensions string

	// LocalStorage is a json file containing user local storage.
	LocalStorage string

	// ExtensionSettings is a json file containing user extension settings.
	ExtensionSettings string

	// ExtensionState is a json file containing user extension state.
	ExtensionState string

	// Types for cloud metadata

	// InstanceID represents the unique ID assigned by the provider to the instance.
	InstanceID string

	// InstanceName is the name given to the instance by the scaling service.
	InstanceName string

	// ImageID is the unique ID associated with the machine image used to start the instance.
	ImageID string

	// InstanceType is the kind of instance in use, depending on its hardware characteristics.
	InstanceType string

	// PlacementRegion is the region or zone where the compute resources exist for a specific cloud provider.
	PlacementRegion string
)

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
