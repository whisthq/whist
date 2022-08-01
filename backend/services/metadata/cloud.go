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

	var (
		awsChan = make(chan error)
		gcpChan = make(chan error)
	)

	// Try to ping the AWS metadata endpoint
	url := aws.EndpointBase + "/instance-id"
	resp, err := httpClient.Get(url)
	awsChan <- err
	resp.Body.Close()

	// Try to ping the GCP metadata endpoint
	url = gcp.EndpointBase + "/instance-id"
	req, err := http.NewRequest(http.MethodGet, url, nil)
	if err != nil {
		CloudMetadata = &gcp.Metadata{}
		return utils.MakeError("failed to create request for GCP endpoint: %s", err)
	}

	// Add necessary headers and send the request
	req.Header.Add("Metadata-Flavor", "Google")
	resp, err = httpClient.Do(req)
	gcpChan <- err
	resp.Body.Close()

	// This select will block until any request to the cloud providers
	// metadata endpoint (or an error) completes. This way we the host
	// service can "know" on which cloud provider its running and start
	// querying for the relevant metadata.
	select {
	case err := <-awsChan:
		if err != nil {
			CloudMetadata = &aws.Metadata{}
			return nil
		}
	case err := <-gcpChan:
		if err != nil {
			CloudMetadata = &gcp.Metadata{}
			return nil
		}
	default:
		return utils.MakeError("no supported metadata endpoint")
	}

	return nil
}
