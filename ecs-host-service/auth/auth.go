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
	"fmt"
	"strings"

	"github.com/dgrijalva/jwt-go"

	"github.com/fractal/fractal/ecs-host-service/fractalcontainer/fctypes"
	"github.com/fractal/fractal/ecs-host-service/utils"
)

// A RawJWT is a custom type we define to make sure we never pass in the wrong
// field to a JWT. This would have prevented some bugs we've ran into before.
type RawJWT string

// A JwtScope is another custom type we define to enforce correctness.
type JwtScope string

var config authConfig = getAuthConfig()

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

// Checks that audience and issuer are correct, then returns the public key.
// This should not be used except as an argument to jwt's parse methods.
func keyFunc(token *jwt.Token) (interface{}, error) {
	// Our Auth0 configuration uses RS256, so we return an rsa public key
	key, err := parsePubPEM(config.VerifyKey)
	if err != nil {
		fmt.Printf("key err %v", err)
	}
	return key, nil
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
		return errors.New("invalid audience")
	}

	// Verify issuer
	issValid := claims["iss"] == config.Iss
	if !issValid {
		return errors.New("invalid issuer")
	}

	// Verify JWT is not expired
	err := claims.Valid()
	if err != nil {
		return err
	}

	return nil
}

// Verify verifies that a JWT is valid.
// If valid, the JWT's claims are returned.
func Verify(accessToken RawJWT) (jwt.MapClaims, error) {
	token, err := jwt.Parse(string(accessToken), keyFunc)
	if err != nil {
		return nil, err
	}

	claims := token.Claims.(jwt.MapClaims)
	err = validateClaims(claims)
	if err != nil {
		return nil, err
	}
	return claims, nil
}

// VerifyWithUserID verifies that a JWT is valid and
// corresponds to the user with userID.
// If valid, the JWT's claims are returned.
func VerifyWithUserID(accessToken RawJWT, userID fctypes.UserID) (jwt.MapClaims, error) {
	claims, err := Verify(accessToken)
	if err != nil {
		return claims, err
	}

	if claims["sub"] != userID {
		return claims, errors.New("userID does not match jwt")
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
