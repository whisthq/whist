package httputils

import (
	mandelboxtypes "github.com/whisthq/whist/backend/services/types"
)

// Request types

// JSONTransportRequest defines the (unauthenticated) `json_transport`
// endpoint.
type JSONTransportRequest struct {
	IP                    string                               `json:"ip"`                             // The public IPv4 address of the instance running the mandelbox
	AppName               mandelboxtypes.AppName               `json:"app_name,omitempty"`             // The app name to spin up (used when running in localdev, but in deployment the app name is set to browsers/chrome).
	ConfigEncryptionToken mandelboxtypes.ConfigEncryptionToken `json:"config_encryption_token"`        // User-specific private encryption token
	JwtAccessToken        string                               `json:"jwt_access_token"`               // User's JWT access token
	MandelboxID           mandelboxtypes.MandelboxID           `json:"mandelbox_id"`                   // MandelboxID, used for the json transport request map
	IsNewConfigToken      bool                                 `json:"is_new_config_encryption_token"` // Flag indicating we should expect a new config encryption token and to skip config decryption this run
	JSONData              mandelboxtypes.JSONData              `json:"json_data"`                      // Arbitrary stringified JSON data to pass to mandelbox
	/*CookiesJSON           mandelboxtypes.Cookies               `json:"cookies,omitempty"`              // The cookies provided by the client-app as JSON string
	BookmarksJSON         mandelboxtypes.Bookmarks             `json:"bookmarks,omitempty"`            // The bookmarks provided by the client-app as JSON string
	Extensions            mandelboxtypes.Extensions            `json:"extensions,omitempty"`           // Extensions provided by the client-app
	Preferences           mandelboxtypes.Preferences           `json:"preferences,omitempty"`          // Preferences provided by the client-app as JSON string
	LocalStorageJSON      mandelboxtypes.LocalStorage          `json:"local_storage,omitempty"`        // Local storage provided by the client-app as JSON string
	ExtensionSettingsJSON mandelboxtypes.ExtensionSettings     `json:"extension_settings,omitempty"`   // Extension settings provided by the client-app as JSON string
	ExtensionStateJSON    mandelboxtypes.ExtensionState        `json:"extension_state,omitempty"`      // Extension state provided by the client-app as JSON string*/
	ResultChan            chan RequestResult                   `json:"-"`                              // Channel to pass the request result between goroutines
}

// JSONTransportRequestResult defines the data returned by the
// `json_transport` endpoint.
type JSONTransportRequestResult struct {
	HostPortForTCP32262 uint16 `json:"port_32262"`
	HostPortForUDP32263 uint16 `json:"port_32263"`
	HostPortForTCP32273 uint16 `json:"port_32273"`
	AesKey              string `json:"aes_key"`
}

// ReturnResult is called to pass the result of a request back to the HTTP
// request handler.
func (s *JSONTransportRequest) ReturnResult(result interface{}, err error) {
	s.ResultChan <- RequestResult{
		Result: result,
		Err:    err,
	}
}

// createResultChan is called to create the Go channel to pass the request
// result back to the HTTP request handler via ReturnResult.
func (s *JSONTransportRequest) CreateResultChan() {
	if s.ResultChan == nil {
		s.ResultChan = make(chan RequestResult)
	}
}

// Mandelbox assign request
type MandelboxAssignRequest struct {
	Regions    []string              `json:"regions"`
	CommitHash string                `json:"client_commit_hash"`
	UserEmail  string                `json:"user_email"`
	Version    string                `json:"version"`
	SessionID  int64                 `json:"session_id"`
	UserID     mandelboxtypes.UserID // The userID is obtained from the access token
	ResultChan chan RequestResult    // Channel to pass the request result between goroutines
}

type MandelboxAssignRequestResult struct {
	IP          string                     `json:"ip"`
	MandelboxID mandelboxtypes.MandelboxID `json:"mandelbox_id"`
	Error       string                     `json:"error"`
}

// ReturnResult is called to pass the result of a request back to the HTTP
// request handler.
func (s *MandelboxAssignRequest) ReturnResult(result interface{}, err error) {
	s.ResultChan <- RequestResult{
		Result: result,
		Err:    err,
	}
}

// createResultChan is called to create the Go channel to pass the request
// result back to the HTTP request handler via ReturnResult.
func (s *MandelboxAssignRequest) CreateResultChan() {
	if s.ResultChan == nil {
		s.ResultChan = make(chan RequestResult)
	}
}
