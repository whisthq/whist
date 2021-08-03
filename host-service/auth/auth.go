/*
Package auth provides functions for validating JWTs sent by the client app.

Currently, it has been tested JWTs generated with our Auth0 configuration.
It should work with other JWTs too, provided that they are signed with the RS256
algorithm.
*/
package auth // import "github.com/fractal/fractal/host-service/auth"

import (
	"encoding/json"
	"strings"
	"time"

	"github.com/golang-jwt/jwt"

	"github.com/MicahParks/keyfunc"

	logger "github.com/fractal/fractal/host-service/fractallogger"
	"github.com/fractal/fractal/host-service/utils"
)

// Audience is an alias for []string with some custom deserialization behavior.
// It is used to store the value of an access token's "aud" claim.
type Audience []string

// Scopes is an alias for []string with some custom deserialization behavior.
// It is used to store the value of an access token's "scope" claim.
type Scopes []string

// FractalClaims is a struct type that models the claims that must be present
// in an Auth0-issued Fractal access token.
type FractalClaims struct {
	jwt.StandardClaims

	// Audience stores the value of the access token's "aud" claim. Inside the
	// token's payload, the value of the "aud" claim can be either a JSON
	// string or list of strings. We implement custom deserialization on the
	// Audience type to handle both of these cases.
	Audience Audience `json:"aud"`

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

// UnmarshalJSON unmarshals a JSON string or list of strings into an *Audience
// type. It overwrites the contents of *audience with the unmarshalled data.
func (audience *Audience) UnmarshalJSON(data []byte) error {
	var aud string

	// Try to unmarshal the input data into a string slice.
	err := json.Unmarshal(data, (*[]string)(audience))

	switch v := err.(type) {
	case *json.UnmarshalTypeError:
		// Ignore *json.UnmarshalTypeErrors, which are returned when the input
		// data cannot be unmarshalled into a string slice. Instead, we will
		// try to unmarshal the data into a string type below.
	default:
		// Return an error if err was non-nil or nil otherwise.
		return v
	}

	// Try to unmarshal the input data into a string.
	if err := json.Unmarshal(data, &aud); err != nil {
		return err
	}

	// Overwrite any pre-existing data in *audience. Avoid creating a new
	// allocation if possible by truncating and reusing the existing slice.
	*audience = append((*audience)[0:0], aud)

	return nil
}

// UnmarshalJSON unmarshals a space-separate string of words into a *Scopes
// type. It overwrites the contents of *scopes with the unmarshalled data.
func (scopes *Scopes) UnmarshalJSON(data []byte) error {
	var s string

	if err := json.Unmarshal(data, &s); err != nil {
		return err
	}

	// The following line is borrowed from
	// https://cs.opensource.google/go/go/+/refs/tags/go1.16.6:src/encoding/json/stream.go;l=271.
	// @djsavvy and @owenniles's best guess is that the (*scopes)[0:0] syntax
	// decreases the likelihood that new memory must be allocated for the
	// data that are written to the slice.
	*scopes = append((*scopes)[0:0], strings.Fields(s)...)

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

// VerifyAudience compares the "aud" claim against cmp. If req is false, this
// method will return true if the value of the "aud" claim matches cmp or is
// unset.
func (claims *FractalClaims) VerifyAudience(cmp string, req bool) bool {
	c := &jwt.MapClaims{"aud": []string(claims.Audience)}
	return c.VerifyAudience(cmp, req)
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
