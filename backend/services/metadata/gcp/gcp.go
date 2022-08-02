package gcp

import (
	"io"
	"net"
	"net/http"
	"time"

	"github.com/whisthq/whist/backend/services/types"
	"github.com/whisthq/whist/backend/services/utils"
)

// EndpointBase is the URL of the metadata endpoint. It is declared as
// a var for testing convenience.
var EndpointBase = "http://metadata.google.internal/computeMetadata/v1/instance/"

// Metadata holds all relevant values that can be extracted from the
// instance's metadata endpoint.
type Metadata struct {
	instanceID   types.InstanceID
	instanceName types.InstanceName
	imageID      types.ImageID
	instanceType types.InstanceType
	region       types.PlacementRegion
	ip           net.IP
}

// GetImageID returns the ID of the image used to launch the instance.
func (gc *Metadata) GetImageID() types.ImageID {
	return gc.imageID
}

// GetInstanceID returns the ID of the instance.
func (gc *Metadata) GetInstanceID() types.InstanceID {
	return gc.instanceID
}

// GetInstanceType returns the type of instance.
func (gc *Metadata) GetInstanceType() types.InstanceType {
	return gc.instanceType
}

// GetInstanceName returns the name of the instance.
func (gc *Metadata) GetInstanceName() types.InstanceName {
	return gc.instanceName
}

// GetPlacementRegion returns the name of the region where the instance is located.
func (gc *Metadata) GetPlacementRegion() types.PlacementRegion {
	return gc.region
}

// GetPublicIpv4 returns the public ip address assigned to the instance.
func (gc *Metadata) GetPublicIpv4() net.IP {
	return gc.ip
}

// GetUserID returns the user ID.
func (gc *Metadata) GetUserID() types.UserID {
	return types.UserID("")
}

// GetMetadata should be called before trying to get any of the metadata values.
// This function makes the initial calls to the endpoint and populates the `Metadata`
// struct.
func (gc *Metadata) GetMetadata() (map[string]string, error) {
	return nil, nil
}

func metadataRetriever(path string) func() (string, error) {
	httpClient := http.Client{
		Timeout: 1 * time.Second,
	}

	url := EndpointBase + path
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
