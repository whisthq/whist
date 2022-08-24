package metadata

import (
	"net/http"
	"net/http/httptest"
	"reflect"
	"testing"

	"github.com/whisthq/whist/backend/services/metadata/aws"
	"github.com/whisthq/whist/backend/services/metadata/gcp"
)

func TestGenerateCloudMetadataRetriever(t *testing.T) {
	var tests = []struct {
		name, provider, path string
		metadataEndpoint     *string
		expected             CloudMetadataRetriever
	}{
		{"AWS successful ping", "aws", "/latest/meta-data/instance-id", &aws.EndpointBase, &aws.Metadata{}},
		{"AWS no response", "aws", "/instance-id", nil, nil},
		{"GCP succesful ping", "gcp", "/id", &gcp.EndpointBase, &gcp.Metadata{}},
		{"GCP no response", "gcp", "/id", nil, nil},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			srv := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
				if r.Method != http.MethodGet {
					http.Error(w, "Method Not Allowed", http.StatusMethodNotAllowed)
				}

				if r.URL.Path != tt.path {
					http.Error(w, "Not Found", http.StatusNotFound)
				}
			}))
			defer srv.Close()

			if tt.metadataEndpoint != nil {
				*tt.metadataEndpoint = srv.URL
			}

			err := GenerateCloudMetadataRetriever()
			if err != nil {
				t.Errorf("did not expect error, got: %s", err)
			}

			if ok := reflect.DeepEqual(CloudMetadata, tt.expected); !ok {
				t.Errorf("metadata retriever was not generated correctly; got type: %T, expected type: %T", CloudMetadata, tt.expected)
			}

			CloudMetadata = nil
		})
	}
}
