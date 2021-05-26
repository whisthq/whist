package authorization // import "github.com/fractal/fractal/ecs-host-service/httpserver"

import (
	"crypto/rsa"
	"errors"

	"github.com/dgrijalva/jwt-go"
)

// Customs claims type representing user info details
// If more claims are added to our configuration (eg. role name),
// they should be added in this struct
type UserInfo struct {
	sub  string // user id
	name string
}

type JWTClaims struct {
	*jwt.StandardClaims
	UserInfo
}

type verifier struct {
	aud       string         // JWT audience
	iss       string         // JWT issuer
	verifyKey *rsa.PublicKey // public key to verify JWT signature
	alg       string         // signing algorithm
}

// verifyKey is a PEM formatted string
func New(aud string, iss string, verifyKey string, alg string) (*verifier, error) {
	v := new(verifier)
	v.aud = aud
	v.iss = iss
	v.alg = alg
	publicKey, err := jwt.ParseRSAPublicKeyFromPEM([]byte(verifyKey))
	v.verifyKey = publicKey
	return v, err
}

// Checks that audience and issuer are correct, then returns the public key
// This should not be used except as an argument to jwt's parse methods
func (v *verifier) keyFunc(token *jwt.Token) (interface{}, error) {
	// Verify audience
	audValid := token.Claims.(jwt.MapClaims).VerifyAudience(v.aud, false)
	if !audValid {
		return token, errors.New("invalid audience")
	}

	// Verify issuer
	issValid := token.Claims.(jwt.MapClaims).VerifyIssuer(v.iss, false)
	if !issValid {
		return token, errors.New("invalid issuer")
	}

	// We only use one algorithm, so simply return the public key
	return v.verifyKey, nil
}

// Returns claims if token verified, errors if not
func (v *verifier) verify(accessToken string) (*JWTClaims, error) {
	token, err := jwt.ParseWithClaims(accessToken, &JWTClaims{}, v.keyFunc)
	if err != nil {
		return nil, err
	}

	claims := token.Claims.(*JWTClaims)
	return claims, nil
}

// Similar to verify, but additionally errors if accessToken does not correspond to the given userId
func (v *verifier) verifyWithUserId(accessToken string, userId string) (*JWTClaims, error) {
	claims, err := v.verify(accessToken)
	if err != nil {
		return claims, err
	}

	if claims.sub != userId {
		return claims, errors.New("userId does not match jwt")
	}

	return claims, nil
}
