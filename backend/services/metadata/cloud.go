package metadata

import (
	"net"
	"net/http"
	"time"

	"github.com/whisthq/whist/backend/services/metadata/aws"
	"github.com/whisthq/whist/backend/services/metadata/gcp"
	"github.com/whisthq/whist/backend/services/types"
	"github.com/whisthq/whist/backend/services/utils"
)

var CloudMetadata CloudMetadataRetriever

type CloudMetadataRetriever interface {
	GetImageID() types.ImageID
	GetInstanceID() types.InstanceID
	GetInstanceType() types.InstanceType
	GetInstanceName() types.InstanceName
	GetPlacementRegion() types.PlacementRegion
	GetPublicIpv4() net.IP
	GetMetadata() (map[string]string, error)
	GetUserID() types.UserID
}

func GenerateCloudMetadataRetriever() error {
	httpClient := http.Client{
		Timeout: 1 * time.Second,
	}

	// First, try to ping the AWS metadata endpoint
	url := aws.EndpointBase + "/instance-id"
	resp, err := httpClient.Get(url)
	defer resp.Body.Close()
	if err == nil {
		CloudMetadata = &aws.Metadata{}
		return nil
	}
	// logger.Warningf("error retrieving data from URL %s: %s", url, err)

	// If the ping to AWS fails, try the GCP metadata endpoint
	url = gcp.EndpointBase + "/instance-id"
	req, err := http.NewRequest(http.MethodGet, url, nil)
	if err != nil {
		CloudMetadata = &gcp.Metadata{}
		return utils.MakeError("failed to create request for GCP endpoint: %s", err)
	}
	req.Header.Add("Metadata-Flavor", "Google")

	resp, err = httpClient.Do(req)
	defer resp.Body.Close()
	if err == nil {
		return nil
	}
	// logger.Warningf("error retrieving data from URL %s: %s", url, err)

	return nil
}
