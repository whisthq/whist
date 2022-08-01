package gcp

import (
	"io"
	"net"
	"net/http"
	"time"

	"github.com/whisthq/whist/backend/services/types"
	"github.com/whisthq/whist/backend/services/utils"
)

const EndpointBase = "http://metadata.google.internal/computeMetadata/v1/instance/"

type Metadata struct {
	instanceID   types.InstanceID
	instanceName types.InstanceName
	imageID      types.ImageID
	instanceType types.InstanceType
	region       types.PlacementRegion
	ip           net.IP
}

func (gc *Metadata) GetImageID() types.ImageID {
	return gc.imageID
}

func (gc *Metadata) GetInstanceID() types.InstanceID {
	return gc.instanceID
}

func (gc *Metadata) GetInstanceType() types.InstanceType {
	return gc.instanceType
}

func (gc *Metadata) GetInstanceName() types.InstanceName {
	return gc.instanceName
}

func (gc *Metadata) GetPlacementRegion() types.PlacementRegion {
	return gc.region
}

func (gc *Metadata) GetPublicIpv4() net.IP {
	return gc.ip
}

func (gc *Metadata) GetUserID() types.UserID {
	return types.UserID("")
}

func (gc *Metadata) GetMetadata() (map[string]string, error) {
	return map[string]string{}, nil
}

func generateGCPMetadataRetriever(path string) func() (string, error) {
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
