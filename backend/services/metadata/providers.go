package metadata

import (
	"net"
	"net/http"
	"net/url"
	"path"
	"time"

	"github.com/whisthq/whist/backend/services/metadata/aws"
	"github.com/whisthq/whist/backend/services/metadata/gcp"
	"github.com/whisthq/whist/backend/services/types"
	"github.com/whisthq/whist/backend/services/utils"
)

// CloudMetadata is a variable exposed by this package so that any
// consumers can get the relevant metadata that was queried from the
// cloud provider.
var CloudMetadata CloudMetadataRetriever

// CloudMetadataRetriever is an interface that abstracts all functionality
// to get metadata from an instance. It allows consumers of this package to
// get the desired metadata without having any provider-specific logic.
type CloudMetadataRetriever interface {
	GetImageID() types.ImageID
	GetInstanceID() types.InstanceID
	GetInstanceType() types.InstanceType
	GetInstanceName() types.InstanceName
	GetPlacementRegion() types.PlacementRegion
	GetPublicIpv4() net.IP
	PopulateMetadata() (map[string]string, error)
	GetUserID() types.UserID
}

// GenerateCloudMetadataRetriever tries to identify the current cloud provider
// the instance is running in by pinging the metadata endpoints. It tries to be
// smart about this by pinging the endpoints in order of provider adoption so that
// the most used provider is first. Unless there is need for creating a retriever
// for a specific provider, this function should be used to generate a new metadata
// retriever instead of directly instanciating a new metadata retriever.
func GenerateCloudMetadataRetriever() error {
	httpClient := http.Client{
		Timeout: 1 * time.Second,
	}

	// The current number of providers supported.
	// This is the number of responses we expect on
	// the provider select.
	const supportedProviders = 2

	var (
		awsChan = make(chan error)
		gcpChan = make(chan error)
	)

	// Try to ping the AWS metadata endpoint
	go func() {
		resourceToPing := "instance-id"
		url, err := url.Parse(aws.EndpointBase)
		if err != nil {
			awsChan <- utils.MakeError("failed to parse metadata endpoint URL: %s", err)
		}
		url.Path = path.Join("latest", "meta-data", resourceToPing)
		resp, err := httpClient.Get(url.String())
		if err != nil {
			awsChan <- err
			return
		}
		if resp.StatusCode != http.StatusOK {
			err = utils.MakeError("did not get a 200 status code")
		}

		awsChan <- err
	}()

	// Try to ping the GCP metadata endpoint
	go func() {
		resourceToPing := "id"
		url, err := url.Parse(gcp.EndpointBase)
		if err != nil {
			gcpChan <- utils.MakeError("failed to parse metadata endpoint URL: %s", err)
		}
		url.Path = path.Join(url.Path, resourceToPing)
		req, err := http.NewRequest(http.MethodGet, url.String(), nil)
		if err != nil {
			gcpChan <- utils.MakeError("failed to create request for GCP endpoint: %s", err)
			return
		}

		// Add necessary headers for GCP and send the request
		req.Header.Add("Metadata-Flavor", "Google")
		resp, err := httpClient.Do(req)
		if err != nil {
			gcpChan <- err
			return
		}

		if resp.StatusCode != http.StatusOK {
			err = utils.MakeError("did not get a 200 status code")
		}

		gcpChan <- err
	}()

	// TODO: Once we add support for more cloud providers, ping their metadata
	// endpoint (or similar feature) here so that the host service can decide
	// which metadata retriever to use. Each ping should happen on a separate
	// goroutine so that the select works as intended.

	// This select will block until any request to the cloud providers
	// metadata endpoint (or an error) completes. This way we the host
	// service can "know" on which cloud provider its running and start
	// querying for the relevant metadata.

	for i := 0; i < supportedProviders; i++ {
		select {
		case err := <-awsChan:
			if err == nil {
				CloudMetadata = &aws.Metadata{}
			}
		case err := <-gcpChan:
			if err == nil {
				CloudMetadata = &gcp.Metadata{}
			}
		}
	}

	return nil
}
