package auth

import (
	"testing"

	"github.com/whisthq/whist/core-go/metadata"
)

// TestGetAuthConfigDev will get the dev app environmet
func TestGetAuthConfigDev(t *testing.T) {
	// getAuthConfig should return authConfigDev
	config := getAuthConfig(metadata.EnvDev)

	if config.Aud != authConfigDev.Aud || config.Iss != authConfigDev.Iss {
		t.Fatalf("error getting auth config for dev. Expected %v, got %v", authConfigDev, config)
	}
}

// TestGetAuthConfigStaging will get the staging app environmet
func TestGetAuthConfigStaging(t *testing.T) {
	// getAuthConfig should return authConfigStaging
	config := getAuthConfig(metadata.EnvStaging)

	if config.Aud != authConfigStaging.Aud || config.Iss != authConfigStaging.Iss {
		t.Fatalf("error getting auth config for staging. Expected %v, got %v", authConfigStaging, config)
	}
}

// TestGetAuthConfigProd will get the prod app environmet
func TestGetAuthConfigProd(t *testing.T) {
	// getAuthConfig should return authConfigProd
	config := getAuthConfig(metadata.EnvProd)

	if config.Aud != authConfigProd.Aud || config.Iss != authConfigProd.Iss {
		t.Fatalf("error getting auth config for prod. Expected %v, got %v", authConfigProd, config)
	}
}
