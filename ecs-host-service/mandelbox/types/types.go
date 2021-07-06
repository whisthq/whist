// Package types simply contains some useful types for the `mandelbox`
// package. We define this package separately so that we can safely pass these
// types around to other packages that `mandelbox` itself might depend
// on.
//
package types // import "github.com/fractal/fractal/ecs-host-service/mandelbox/types"

// We define special types for the following string types for all the benefits
// of type safety, including making sure we never switch Docker and Fractal
// IDs, for instance.

// A MandelboxID is a random string that the webserver creates for each
// mandelbox. We need some sort of identifier for each mandelbox, and we need
// it _before_ Docker gives us back the runtime Docker ID for the mandelbox.
// TODO: change this type to a UUID via github.com/google/uuid
type MandelboxID string

// A DockerID is provided by Docker at mandelbox creation time.
type DockerID string

// AppName is defined as its own type so, in the future, we can always easily
// enforce that it is part of a limited set of values.
type AppName string

// UserID is defined as its own type as well so the compiler can check argument
// orders, etc. more effectively.
type UserID string

// ConfigEncryptionToken is defined as its own type for similar reasons.
type ConfigEncryptionToken string

// ClientAppAccessToken is defined as its own type for similar reasons.
type ClientAppAccessToken string
