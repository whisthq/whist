package aws

import (
	"net/http"
	"net/http/httptest"
	"path"
	"reflect"
	"testing"
)

func TestGetMetadata(t *testing.T) {
	var validMetadata = map[string]string{
		"aws.image_id":      "test-ami-id",
		"aws.instance_id":   "test-instance-id",
		"aws.instance_name": "test-instance-name",
		"aws.instance_type": "test-instance-type",
		"aws.region":        "test-region",
		"aws.ip":            "0.0.0.0",
	}

	var tests = []struct {
		name, overrideKey string
		error             bool
		expected          map[string]string
	}{
		{"AWS all good", "", false, validMetadata},
		{"AWS no ami-id", "aws.ami_id", true, map[string]string{}},
		{"AWS no instance-id", "aws.instance_id", true, map[string]string{}},
		{"AWS no instance-type", "aws.instance_type", true, map[string]string{}},
		{"AWS no placement/region", "aws.region", true, map[string]string{}},
		{"AWS no public-ipv4", "aws.ip", true, map[string]string{}},
		{"AWS no instance-name", "aws.instance_name", true, map[string]string{}},
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

				basePath := "/latest/meta-data/"
				var result string
				switch r.URL.Path {
				case path.Join(basePath, "ami-id"):
					result = validMetadata["aws.image_id"]
				case path.Join(basePath, "instance-id"):
					result = validMetadata["aws.instance_id"]
				case path.Join(basePath, "instance-type"):
					result = validMetadata["aws.instance_type"]
				case path.Join(basePath, "placement/region"):
					result = validMetadata["aws.region"]
				case path.Join(basePath, "public-ipv4"):
					result = validMetadata["aws.ip"]
				}
				w.Write([]byte(result))
			}))
			defer srv.Close()

			EndpointBase = srv.URL

			// Use a mock function to avoid using the AWS SDK.
			getInstanceName = func(instanceID, region string) (string, error) {
				return "test-instance-name", nil
			}

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
		name, resource       string
		overrideEndpoint     string
		useEndpointBase, err bool
		expected             string
	}{
		{"Valid resource", "ami-id", "", true, false, "test_ami"},
		{"Empty resource", "", "", true, true, ""},
		{"Invalid endpoint base", "", ":", false, true, ""},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			srv := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
				if r.Method != http.MethodGet {
					http.Error(w, "Method Not Allowed", http.StatusMethodNotAllowed)
				}

				if r.URL.Path != "/latest/meta-data/ami-id" {
					t.Logf("Returning not found")
					http.Error(w, http.StatusText(http.StatusNotFound), http.StatusNotFound)
				}

				w.Write([]byte("test_ami"))
			}))

			defer srv.Close()

			if tt.useEndpointBase {
				EndpointBase = srv.URL
			} else if tt.overrideEndpoint != "" {
				EndpointBase = tt.overrideEndpoint
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
