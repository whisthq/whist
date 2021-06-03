package auth // import "github.com/fractal/fractal/ecs-host-service/httpserver"

import (
	"crypto/rsa"
	"crypto/x509"
	"encoding/pem"
	"errors"
	"fmt"

	"github.com/dgrijalva/jwt-go"
)

var AUD string
var ISS string
var VERIFY_KEY string

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

// Checks that audience and issuer are correct, then returns the public key
// This should not be used except as an argument to jwt's parse methods
func keyFunc(token *jwt.Token) (interface{}, error) {
	// We only use one algorithm (RS256), so simply return the public key
	key, err := parsePubPEM(VERIFY_KEY)
	if err != nil {
		fmt.Printf("key err %v", err)
	}
	return key, nil
}

func audContains(audSlice []interface{}, aud interface{}) bool {
	for _, v := range audSlice {
		if v == aud {
			return true
		}
	}
	return false
}

func validateClaims(claims jwt.MapClaims) error {
	// Verify audience
	audSlice := claims["aud"].([]interface{})
	audValid := audContains(audSlice, AUD)
	if !audValid {
		return errors.New("invalid audience")
	}

	// Verify issuer
	issValid := claims["iss"] == ISS
	if !issValid {
		fmt.Printf("iss invalid")
		return errors.New("invalid issuer")
	}

	return nil
}

// Returns claims if token verified, errors if not
func Verify(accessToken string) (jwt.MapClaims, error) {
	token, err := jwt.Parse(accessToken, keyFunc)
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

// Similar to verify, but additionally errors if accessToken does not correspond to the given userId
func VerifyWithUserId(accessToken string, userId string) (jwt.MapClaims, error) {
	claims, err := Verify(accessToken)
	if err != nil {
		return claims, err
	}

	if claims["sub"] != userId {
		return claims, errors.New("userID does not match jwt")
	}

	return claims, nil
}
