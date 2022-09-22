package gcp

import (
	"io"
	"net"
	"net/http"
	"net/url"
	"path"
	"time"

	"github.com/hashicorp/go-multierror"
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

// PopulateMetadata should be called before trying to get any of the metadata values.
// This function makes the initial calls to the endpoint and populates the `Metadata`
// struct.
func (gc *Metadata) PopulateMetadata() (map[string]string, *multierror.Error) {
	var errors *multierror.Error
	var metadata = make(map[string]string)

	if imageID, err := metadataRetriever("image"); err != nil {
		multierror.Append(errors, utils.MakeError("failed to get gcp metadata field image: %s", err))
	} else {
		metadata["gcp.image_id"] = imageID
		gc.imageID = types.ImageID(imageID)
	}

	if instanceID, err := metadataRetriever("id"); err != nil {
		multierror.Append(errors, utils.MakeError("failed to get gcp metadata field id: %s", err))
	} else {
		metadata["gcp.instance_id"] = instanceID
		gc.instanceID = types.InstanceID(instanceID)
	}

	if instanceType, err := metadataRetriever("machine-type"); err != nil {
		multierror.Append(errors, utils.MakeError("failed to get gcp metadata field machine-type: %s", err))
	} else {
		metadata["gcp.instance_type"] = instanceType
		gc.instanceType = types.InstanceType(instanceType)
	}

	if region, err := metadataRetriever("zone"); err != nil {
		multierror.Append(errors, utils.MakeError("failed to get gcp metadata field zone: %s", err))
	} else {
		metadata["gcp.region"] = region
		gc.region = types.PlacementRegion(region)
	}

	if ip, err := metadataRetriever("network-interfaces/0/ip"); err != nil {
		multierror.Append(errors, utils.MakeError("failed to get gcp metadata field ip: %s", err))
	} else {
		metadata["gcp.ip"] = ip
		gc.ip = net.ParseIP(ip).To4()
	}

	if instanceName, err := metadataRetriever("name"); err != nil {
		multierror.Append(errors, utils.MakeError("failed to get gcp metadata field name: %s", err))
	} else {
		metadata["gcp.instance_name"] = instanceName
		gc.instanceName = types.InstanceName(instanceName)
	}

	return metadata, errors
}

func metadataRetriever(resource string) (string, error) {
	httpClient := http.Client{
		Timeout: 1 * time.Second,
	}
	u, err := url.Parse(EndpointBase)
	if err != nil {
		return "", utils.MakeError("failed to parse metadata endpoint URL: %s", err)
	}

	u.Path = path.Join(u.Path, resource)
	req, err := http.NewRequest(http.MethodGet, u.String(), nil)
	if err != nil {
		return "", utils.MakeError("failed to create request for GCP endpoint: %s", err)
	}

	req.Header.Add("Metadata-Flavor", "Google")
	resp, err := httpClient.Do(req)
	if err != nil {
		return "", utils.MakeError("error retrieving data from URL %s: %v. Is the service running on a GCP VM instance?", u.String(), err)
	}
	defer resp.Body.Close()

	body, err := io.ReadAll(resp.Body)
	if err != nil {
		return "", utils.MakeError("error reading response body from URL %s: %v", u.String(), err)
	}
	if resp.StatusCode != 200 {
		return "", utils.MakeError("got non-200 response from URL %s: %s", u.String(), resp.Status)
	}
	return string(body), nil
}
