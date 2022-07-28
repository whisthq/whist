package gcp

import (
	"io"
	"net/http"
	"time"

	"github.com/whisthq/whist/backend/services/utils"
)

const GCPendpointBase = "http://metadata.google.internal/computeMetadata/v1/instance/"

func generateGCPMetadataRetriever(path string) func() (string, error) {
	httpClient := http.Client{
		Timeout: 1 * time.Second,
	}

	url := GCPendpointBase + path
	return func() (string, error) {
		resp, err := httpClient.Get(url)
		if err != nil {
			return "", utils.MakeError("error retrieving data from URL %s: %v. Is the service running on a GCP VM instance?", url, err)
		}
		defer resp.Body.Close()

		body, err := io.ReadAll(resp.Body)
		if err != nil {
			return string(body), utils.MakeError("error reading response body from URL %s: %v", url, err)
		}
		if resp.StatusCode != 200 {
			return string(body), utils.MakeError("got non-200 response from URL %s: %s", url, resp.Status)
		}
		return string(body), nil
	}
}
