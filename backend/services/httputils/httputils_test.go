package httputils

import (
	"net/http"
	"net/http/httptest"
	"reflect"
	"strings"
	"testing"

	"github.com/google/uuid"
	"github.com/whisthq/whist/backend/services/types"
)

func TestGetAccessToken(t *testing.T) {
	var tests = []struct {
		name, header, expected string
		err                    bool
	}{
		{"Valid Authorization header", "Bearer test_valid_token", "test_valid_token", false},
		{"Malformed Authorization header", "test_malformed_token", "", true},
		{"Empty Authorization header", "", "", true},
		{"Undefined Authorization header", "undefined", "", true},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			r := httptest.NewRequest(http.MethodPost, "https://localhost", nil)
			r.Header.Add("Authorization", tt.header)
			token, err := GetAccessToken(r)
			if err != nil && !tt.err {
				t.Errorf("did not expect error, got %s", err)
			}

			if token != tt.expected {
				t.Errorf("expected token to be %s, got %s", tt.expected, token)
			}
		})
	}
}

func TestParseRequest(t *testing.T) {
	var tests = []struct {
		name     string
		request  ServerRequest
		jsonBody string
		expected ServerRequest
	}{
		{"Valid assign request", &MandelboxAssignRequest{}, `{
			"regions": [
				"us-east-1"
			],
			"client_commit_hash": "test_sha",
			"user_email": "user@whist.com",
			"version": "1.0.0",
			"session_id": 1234567890
		}`, &MandelboxAssignRequest{
			Regions: []string{
				"us-east-1",
			},
			CommitHash: "test_sha",
			UserEmail:  "user@whist.com",
			Version:    "1.0.0",
			SessionID:  1234567890,
		}},
		{"Empty assign request", &MandelboxAssignRequest{}, `{}`, &MandelboxAssignRequest{}},
		{"Valid JSON transport request", &JSONTransportRequest{}, `{
			"ip": "0.0.0.0",
			"app_name": "chrome",
			"config_encryption_token": "test_config_encryption",
			"mandelbox_id": "e0aa0e21-3350-49b6-95a6-152dd2a13e3c",
			"is_new_config_encryption_token": false,
			"json_data": "test_json_data",
			"browser_data": "test_browser_data"

		}`, &JSONTransportRequest{
			IP:                    "0.0.0.0",
			AppName:               "CHROME",
			ConfigEncryptionToken: "test_config_encryption",
			MandelboxID:           types.MandelboxID(uuid.MustParse("e0aa0e21-3350-49b6-95a6-152dd2a13e3c")),
			IsNewConfigToken:      false,
			JSONData:              types.JSONData(`"location":"Americas/NewYork"`),
			BrowserData:           types.BrowserData(`"cookies": "test_cookies"`),
		}},
		{"Empty JSON transport request", &JSONTransportRequest{}, `{}`, &JSONTransportRequest{}},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			body := strings.NewReader(tt.jsonBody)
			w := httptest.NewRecorder()
			r := httptest.NewRequest(http.MethodPost, "https://localhost", body)

			_, err := ParseRequest(w, r, tt.request)
			if err != nil {
				t.Errorf("Did not expect error, got %s", err)
			}

			if ok := reflect.DeepEqual(reflect.TypeOf(tt.request).Elem(), reflect.TypeOf(tt.expected).Elem()); !ok {
				t.Errorf("Expected request to be parsed to %v, got %v", tt.expected, tt.request)
			}
		})
	}
}

func TestVerifyRequestType(t *testing.T) {
	var tests = []struct {
		name, method string
	}{
		{"GET Request", http.MethodGet},
		{"POST Request", http.MethodPost},
		{"PUT Request", http.MethodPut},
	}

	methodsToTest := []string{
		http.MethodHead,
		http.MethodOptions,
		http.MethodGet,
		http.MethodPost,
		http.MethodPut,
		http.MethodDelete,
		http.MethodPatch,
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			for _, method := range methodsToTest {
				w := httptest.NewRecorder()
				r := httptest.NewRequest(method, "https://localhost", nil)

				err := VerifyRequestType(w, r, tt.method)
				if err != nil && tt.method == method {
					t.Errorf("Did not expect error, got %s", err)
				}
			}
		})
	}
}

func TestEnableCORS(t *testing.T) {

}

func TestInitializeTLS(t *testing.T) {

}
