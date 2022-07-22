package httputils

import (
	"net/http"
	"net/http/httptest"
	"testing"
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
}

func TestVerifyRequestType(t *testing.T) {

}

func TestEnableCORS(t *testing.T) {

}

func TestInitializeTLS(t *testing.T) {

}
