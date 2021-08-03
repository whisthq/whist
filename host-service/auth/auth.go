/*
Package auth provides functions for validating JWTs sent by the client app.

Currently, it has been tested JWTs generated with our Auth0 configuration.
It should work with other JWTs too, provided that they are signed with the RS256
algorithm.
*/
package auth // import "github.com/fractal/fractal/host-service/auth"

import (
	"strings"
	"time"

	"github.com/golang-jwt/jwt"

	"github.com/MicahParks/keyfunc"

	logger "github.com/fractal/fractal/host-service/fractallogger"
	"github.com/fractal/fractal/host-service/utils"
)

// Scopes is an alias for []string with some custom deserialization behavior.
// It is used to store the value of an access token's "scope" claim.
type Scopes []string

// FractalClaims is a struct type that models the claims that must be present
// in an Auth0-issued Fractal access token.
type FractalClaims struct {
	jwt.StandardClaims

	// Scopes stores the value of the access token's "scope" claim. The value
	// of the "scope" claim is a string of one or more space-separated words.
	// The *Scopes type implements the encoding.TextUnmarshaler interface such
	// that the string of space-separated words is deserialized into a string
	// slice.
	Scopes Scopes `json:"scope"`
}

var config authConfig = getAuthConfig()
var jwks *keyfunc.JWKs

func init() {
	refreshInterval := time.Hour * 1
	refreshUnknown := true
	var err error // don't want to shadow jwks accidentally

	jwks, err = keyfunc.Get(config.getJwksURL(), keyfunc.Options{
		RefreshInterval: &refreshInterval,
		RefreshErrorHandler: func(err error) {
			logger.Errorf("Error refreshing JWKs: %s", err)
		},
		RefreshUnknownKID: &refreshUnknown,
	})
	if err != nil {
		// Can do a "real" panic since we're in an init function
		logger.Panicf(nil, "Error getting JWKs on startup: %s", err)
	}
	logger.Infof("Successfully got JWKs from %s on startup.", config.getJwksURL())
}

// UnmarshalText implements the encoding.TextUnmarshaler interface on the
// *Scopes type such that a string of space-separated words is serialized into
// a string slice. UnmarshalText overwrites the contents of *scopes with the
// unmarshaled data.
func (scopes *Scopes) UnmarshalText(data []byte) error {
	if scopes == nil {
		return utils.MakeError("auth.Scopes: UnmarshalText on nil pointer")
	}

	// The following line is borrowed from
	// https://cs.opensource.google/go/go/+/refs/tags/go1.16.6:src/encoding/json/stream.go;l=271.
	// @djsavvy and @owenniles's best guess is that the (*scopes)[0:0] syntax
	// decreases the likelihood that new memory must be allocated for the
	// data that are written to the slice.
	*scopes = append((*scopes)[0:0], strings.Fields(string(data))...)

	return nil
}

// Verify parses an raw access token string, verifies the token's signature,
// ensures that it is valid at the current moment in time, and checks that it
// was issued by the proper issuer for the proper audience. It returns a
// pointer to a FractalClaims type containing the values of its claims if all
// checks are successful.
func Verify(tokenString string) (*FractalClaims, error) {
	claims := new(FractalClaims)
	_, err := jwt.ParseWithClaims(tokenString, claims, jwks.KeyFunc)

	if err != nil {
		return nil, err
	}

	if !claims.VerifyAudience(config.Aud, true) {
		return nil, jwt.NewValidationError(
			utils.Sprintf("Bad audience %s", claims.Audience),
			jwt.ValidationErrorAudience,
		)
	}

	if !claims.VerifyIssuer(config.Iss, true) {
		return nil, jwt.NewValidationError(
			utils.Sprintf("Bad issuer %s", claims.Issuer),
			jwt.ValidationErrorIssuer,
		)
	}

	return claims, nil
}

// VerifyScope returns true if claims.Scopes contains the requested scope and
// false otherwise. This function's name and type signature is inspired by
// those of the Verify* methods defined on jwt.StandardClaims.
func (claims *FractalClaims) VerifyScope(scope string) bool {
	for _, s := range claims.Scopes {
		if s == scope {
			return true
		}
	}

	return false
}
