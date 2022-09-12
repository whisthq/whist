package helpers

import (
	"github.com/whisthq/whist/backend/services/scaling-service/hosts"
	aws "github.com/whisthq/whist/backend/services/scaling-service/hosts/aws"
	gcp "github.com/whisthq/whist/backend/services/scaling-service/hosts/gcp"
	"github.com/whisthq/whist/backend/services/types"
)

func GetHostFromProvider(provider types.CloudProvider) hosts.HostHandler {
	switch provider {
	case "AWS":
		return &aws.AWSHost{}
	case "GCP":
		return &gcp.GCPHost{}
	default:
		return &aws.AWSHost{}
	}
}
