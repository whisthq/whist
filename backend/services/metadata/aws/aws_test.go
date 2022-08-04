package aws

import (
	"net/http"
	"net/http/httptest"
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

				switch r.URL.Path {
				case "/ami-id":
					w.Write([]byte(validMetadata["aws.image_id"]))
				case "/instance-id":
					w.Write([]byte(validMetadata["aws.instance_id"]))
				case "/instance-type":
					w.Write([]byte(validMetadata["aws.instance_type"]))
				case "/placement/region":
					w.Write([]byte(validMetadata["aws.region"]))
				case "/public-ipv4":
					w.Write([]byte(validMetadata["aws.ip"]))
				default:
					http.Error(w, "Not Found", http.StatusNotFound)
				}
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

}
