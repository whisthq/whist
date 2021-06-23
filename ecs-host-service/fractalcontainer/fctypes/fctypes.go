// Package fctypes simply contains some useful types for the `fractalcontainer`
// package. We define this package separately so that we can safely pass these
// types around to other packages that `fractalcontainer` itself might depend
// on.
//
package fctypes // import "github.com/fractal/fractal/ecs-host-service/fractalcontainer/fctypes"

// We define special types for the following string types for all the benefits
// of type safety, including making sure we never switch Docker and Fractal
// IDs, for instance.

// A FractalID is a random string that the webserver creates for each
// container. We need some sort of identifier for each container, and we need
// it _before_ Docker gives us back the runtime Docker ID for the container.
// TODO: change this type to a UUID via github.com/google/uuid
type FractalID string

// A DockerID is provided by Docker at container creation time.
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
