package subscriptions

import (
	"net/http"
	"net/http/httptest"
	"reflect"
	"testing"

	"github.com/whisthq/whist/backend/services/metadata"
)

func TestRoundTrip(t *testing.T) {
	var tests = []struct {
		name, method, url, secret string
	}{
		{"Get req", http.MethodGet, "http://localhost:8080", "admin_secret"},
		{"Post req", http.MethodPost, "http://localhost:8080", "admin_secret"},
		{"Put req", http.MethodPut, "http://localhost:8080", "admin_secret"},
		{"Delete req", http.MethodDelete, "http://localhost:8080", "admin_secret"},
		{"Empty secret", http.MethodGet, "http://localhost:8080", ""},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			secretTransport := withAdminSecretTransport{}
			secretTransport.AdminSecret = tt.secret

			req := httptest.NewRequest(tt.method, tt.url, nil)
			_, err := secretTransport.RoundTrip(req)
			if err != nil {
				t.Errorf("failed to set roundtripper: %s", err)
			}

			if req.Header.Get("Content-Type") != "application/json" {
				t.Errorf("content type header not set by roundtripper")
			}

			if req.Header.Get("x-hasura-admin-secret") != tt.secret {
				t.Errorf("hasura secret header not set by roundtripper")
			}
		})
	}
}

func TestInitialize(t *testing.T) {
	var tests = []struct {
		name         string
		env          metadata.AppEnvironment
		useConfigDB  bool
		expectParams HasuraParams
	}{
		{"Whist database", metadata.EnvLocalDev, false, HasuraParams{URL: "http://localhost:8080/v1/graphql", AccessKey: "hasura"}},
		{"Config DB", metadata.EnvLocalDev, true, HasuraParams{URL: "http://localhost:8082/v1/graphql", AccessKey: "hasura"}},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			Enabled = true
			client := &GraphQLClient{}

			err := client.Initialize(tt.useConfigDB)
			if err != nil {
				t.Errorf("did not expect err, got %s", err)
			}

			ok := reflect.DeepEqual(client.GetParams(), tt.expectParams)
			if !ok {
				t.Errorf("incorrect parameters, want: %v, got %v", tt.expectParams, client.GetParams())
			}
		})
	}
}
