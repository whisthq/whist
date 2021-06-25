/*
Package auth provides functions for validating JWTs sent by the client app.

Currently, it has been tested JWTs generated with our Auth0 configuration.
It should work with other JWTs too, provided that they are signed with the RS256
algorithm.
*/
package auth // import "github.com/fractal/fractal/ecs-host-service/auth"

import (
	"crypto/rsa"
	"crypto/x509"
	"encoding/pem"
	"errors"
	"strings"
	"time"

	"github.com/dgrijalva/jwt-go"

	"github.com/MicahParks/keyfunc"

	"github.com/fractal/fractal/ecs-host-service/fractalcontainer/fctypes"
	logger "github.com/fractal/fractal/ecs-host-service/fractallogger"
	"github.com/fractal/fractal/ecs-host-service/utils"
)

// A RawJWT is a custom type we define to make sure we never pass in the wrong
// field to a JWT. This would have prevented some bugs we've ran into before.
type RawJWT string

// A JwtScope is another custom type we define to enforce correctness.
type JwtScope string

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

func parsePubPEM(pubPEM string) (*rsa.PublicKey, error) {
	block, _ := pem.Decode([]byte(pubPEM))
	if block == nil {
		return nil, errors.New("failed to parse PEM block containing the public key")
	}

	pub, err := x509.ParsePKIXPublicKey(block.Bytes)
	if err != nil {
		return nil, errors.New("failed to parse DER encoded public key: " + err.Error())
	}

	switch pub := pub.(type) {
	case *rsa.PublicKey:
		return pub, nil
	default:
		return nil, errors.New("unknown PEM format or not public key")
	}
}

func validateClaims(claims jwt.MapClaims) error {
	// Verify audience
	audSlice := claims["aud"]
	var audValid bool
	switch audSlice.(type) {
	case string:
		audValid = audSlice == config.Aud
	default:
		// If not a string, must be an array.
		audSlice := audSlice.([]interface{})
		audValid = utils.SliceContains(audSlice, config.Aud)
	}
	if !audValid {
		return utils.MakeError(`Invalid JWT audience. Expected "%s", Got "%s".`, config.Aud, audSlice)
	}

	// Verify issuer
	issValid := claims["iss"] == config.Iss
	if !issValid {
		return utils.MakeError(`Invalid JWT issuer. Expected "%s", Got "%s"`, config.Iss, claims["iss"])
	}

	// Verify JWT is not expired
	err := claims.Valid()
	if err != nil {
		return utils.MakeError("Couldn't validate JWT claims: %s", err)
	}

	return nil
}

// Verify verifies that a JWT is valid.
// If valid, the JWT's claims are returned.
func Verify(accessToken RawJWT) (jwt.MapClaims, error) {
	token, err := jwt.Parse(string(accessToken), jwks.KeyFunc)
	if err != nil {
		return nil, utils.MakeError("Error parsing JWT: %s", err)
	}

	claims := token.Claims.(jwt.MapClaims)
	err = validateClaims(claims)
	if err != nil {
		return nil, utils.MakeError("Error verifying JWT since couldn't validate claims: %s", err)
	}
	return claims, nil
}

// VerifyWithUserID verifies that a JWT is valid and corresponds to the user
// with userID. If valid, the JWT's claims are returned.
func VerifyWithUserID(accessToken RawJWT, userID fctypes.UserID) (jwt.MapClaims, error) {
	claims, err := Verify(accessToken)
	if err != nil {
		return nil, err
	}

	// Note that we need to do this cast, since `claims["sub"]` is an
	// interface{}. Therefore, naively comparing `claims["sub"]` with userID will
	// always result in non-equality, even if they are equal strings.
	jwtID, ok := claims["sub"].(string)
	if !ok {
		return nil, utils.MakeError("Couldn't cast JWT-provided sub %v into string!", claims["sub"])
	}
	if jwtID != string(userID) {
		return nil, utils.MakeError(`userID "%s" does not match JWT's provided "%s"`, userID, jwtID)
	}

	return claims, nil
}

// HasScope returns true if the given jwt.MapClaims has
// a scope matching a string, and false if not.
func HasScope(claims jwt.MapClaims, scope JwtScope) bool {
	scopes, ok := claims["scope"].(string)
	if !ok {
		return false
	}
	scopeSlice := strings.Split(scopes, " ")
	for _, s := range scopeSlice {
		if s == string(scope) {
			return true
		}
	}
	return false
}
