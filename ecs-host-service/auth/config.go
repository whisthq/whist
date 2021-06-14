package auth

import (
	"os"
	"strings"
)

type AuthConfig struct {
	// JWT audience. Identifies the serivce that accepts the token.
	Aud string
	// JWT issuer. The issuing server.
	Iss string
	// PEM-formatted public key used to verify JWT signer.
	VerifyKey string
}

var AuthConfigDev = AuthConfig{
	Aud: "https://api.fractal.co",
	Iss: "https://fractal-dev.us.auth0.com/",
	VerifyKey: `-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAv9vqBRqFJZZOvc3Zoq60
i8NRwYuTSiRnWjw4qLEPtHtNG0JVSDwtdcgcy2txR+/T7OA58wHQypMjXcd2A1Wq
ufLXuKW8n7z8fioa45cdAhTLMYnsiimGwTM4Q504GFo8FntZDzjfxkFft0Exf/G8
f15+6tHd5UyHGOehRRly1OzewQhvey+GbUitTG/x9Q34Zw80cbuF/4/+peyb0zAH
G2DGzIZEOArfPTxoeFeyzNCvoq5dM6KEzu1nY1EhXxCON1nBMN6iCPFaQv15Yo8Z
DM2uoZfTzKYVj5PJn3KbUS7oRhsA2u+ZArChmmgymzKgOFeTeBoCzwFF2Xf8pj4S
2wIDAQAB
-----END PUBLIC KEY-----
`,
}

var AuthConfigStaging = AuthConfig{
	Aud: "https://api.fractal.co",
	Iss: "https://fractal-staging.us.auth0.com/",
	VerifyKey: `-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAtBHeP2sS+YqGs7hLqxxw
0ebUQ+8E85+OJsGIu1PXQpqBybN2hMpUMHIxCPzipPfGPlu+fb5tIL/Qvect9ocT
HcUIAaUNOHIk9rZ/ZtHrf740RrzegVNe+HlLU+7kTV60wwp9sMdbTbxGMqWAUj4d
eZXHo4hq+zYSvAAcAJbFrR7GaYivsqRirjHksPkJHkiOiC4KSfKSX9UPRp+gtq/L
jze3BhAswBchJ9EDE1TevXOBB9397O0Mz6IuE7uC1HpQxNhTnx/gZDkwsmN1giEW
qSDfH3aQNrDkDvgM8rCXTxnCePndcHNrEHo+RrhEjPsRkmuMjJYnyladKrgz1vkS
bwIDAQAB
-----END PUBLIC KEY-----`,
}

var AuthConfigProd = AuthConfig{
	Aud: "https://api.fractal.co",
	Iss: "https://fractal-prod.us.auth0.com/",
	VerifyKey: `-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAzBiFN5J3Q7DeSKR6TRtk
2z906kFX1/grZNy8o2OqUVGD6WPU9VlNh531TBYi40K+rLUV4WQNscZZ5V9Sri+k
U+gMbeYAlhI2hxIYrQWHjJBnI+1V/EZmlBdJ2uhEiFroLKwTb6qaGijUc0AAqxeq
j46M2D+EDa11Kuk08hsDNUys4wCAsXGeDh3yjIWjCu4xtDGCy0oaztSY0ZEd7J+D
Pa1lINCpNB5NcFCyH5SGp55nBsEO3FmIwHELUCwg4LZHsQ09UC57da58LDlvrYIg
7TDrDnikaVAG160AI5cPlRTBqU8ehH13zIgAAxJfvpQxPViP55hp81CdnEe9fEdn
CQIDAQAB
-----END PUBLIC KEY-----
`,
}

func getAuthConfig() AuthConfig {
	env := strings.ToLower(os.Getenv("APP_ENV"))
	switch env {
	case "development", "dev":
		return AuthConfigDev
	case "staging":
		return AuthConfigStaging
	case "production", "prod":
		return AuthConfigProd
	default:
		return AuthConfigDev
	}
}
