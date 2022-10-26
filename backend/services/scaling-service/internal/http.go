// http.go

package internal

import "net/http"

// The AdminSecret type implements the RoundTripper interface from the net/http
// package. Set a http.Client's Transport field to a value of type AdminSecret
// to set the X-Hasura-Admin-Secret header on all outgoing HTTP requests to that
// value.
type AdminSecret string

// AdminSecretClient returns an *http.Client that sets the X-Hasura-Admin-Secret
// header on all outgoing HTTP requests to s.
func AdminSecretClient(s string) *http.Client {
	return &http.Client{Transport: AdminSecret(s)}
}

// RoundTrip sets the X-Hasura-Admin-Secret header on all outgoing HTTP
// requests.
func (t AdminSecret) RoundTrip(req *http.Request) (*http.Response, error) {
	copy := req.Clone(req.Context())
	copy.Header.Add("X-Hasura-Admin-Secret", string(t))

	return http.DefaultTransport.RoundTrip(copy)
}
