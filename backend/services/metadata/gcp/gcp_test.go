package gcp

import (
	"net/http"
	"net/http/httptest"
	"reflect"
	"testing"
)

func TestGetMetadata(t *testing.T) {
	var validMetadata = map[string]string{
		"gcp.image_id":      "test-image",
		"gcp.instance_id":   "test-instance-id",
		"gcp.instance_name": "test-instance-name",
		"gcp.instance_type": "test-instance-type",
		"gcp.region":        "test-region",
		"gcp.ip":            "0.0.0.0",
	}

	var tests = []struct {
		name, overrideKey string
		error             bool
		expected          map[string]string
	}{
		{"GCP all good", "", false, validMetadata},
		{"GCP no image", "gcp.ami_id", true, map[string]string{}},
		{"GCP no instance-id", "gcp.instance_id", true, map[string]string{}},
		{"GCP no instance-type", "gcp.instance_type", true, map[string]string{}},
		{"GCP no placement/region", "gcp.region", true, map[string]string{}},
		{"GCP no public-ipv4", "gcp.ip", true, map[string]string{}},
		{"GCP no instance-name", "gcp.instance_name", true, map[string]string{}},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			srv := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
				if r.Method != http.MethodGet {
					http.Error(w, "Method Not Allowed", http.StatusMethodNotAllowed)
				}

				for k := range validMetadata {
					if k == tt.overrideKey {
						http.Error(w, "Not Found", http.StatusNotFound)
					}
				}

				var result string
				switch r.URL.Path {
				case "/image":
					result = validMetadata["gcp.image_id"]
				case "/id":
					result = validMetadata["gcp.instance_id"]
				case "/machine-type":
					result = validMetadata["gcp.instance_type"]
				case "/zone":
					result = validMetadata["gcp.region"]
				case "/network-interfaces/0/ip":
					result = validMetadata["gcp.ip"]
				case "/name":
					result = validMetadata["gcp.instance_name"]
				}
				w.Write([]byte(result))
			}))
			defer srv.Close()

			EndpointBase = srv.URL

			retriever := Metadata{}
			metadataMap, err := retriever.PopulateMetadata()
			if err != nil && !tt.error {
				t.Errorf("did not expect error, got: %s", err)
			} else if tt.error {
				return
			}

			if ok := reflect.DeepEqual(tt.expected, metadataMap); !ok {
				t.Errorf("failed to get metadata map, expected %v, got %v", tt.expected, metadataMap)
			}
		})
	}
}

func TestMetadataRetriever(t *testing.T) {
	var tests = []struct {
		name, resource   string
		overrideEndpoint string
		err              bool
		expected         string
	}{
		{"Valid resource", "image", "", false, "test_image"},
		{"Empty resource", "", "", true, ""},
		{"Invalid endpoint base", "image", ":", true, ""},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			srv := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
				if r.Method != http.MethodGet {
					http.Error(w, "Method Not Allowed", http.StatusMethodNotAllowed)
				}

				if r.URL.Path != "/image" {
					t.Logf("Returning not found")
					http.Error(w, http.StatusText(http.StatusNotFound), http.StatusNotFound)
				}

				w.Write([]byte("test_image"))
			}))

			defer srv.Close()

			if tt.overrideEndpoint != "" {
				EndpointBase = tt.overrideEndpoint
			} else {
				EndpointBase = srv.URL
			}

			metadata, err := metadataRetriever(tt.resource)
			t.Log(err)
			if err != nil && !tt.err {
				t.Errorf("did not expect error, got: %s", err)
			}

			if ok := reflect.DeepEqual(metadata, tt.expected); !ok {
				t.Errorf("failed to get metadata, expected %v, got %v", tt.expected, metadata)
			}
		})
	}
}
