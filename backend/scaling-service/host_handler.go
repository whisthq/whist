package hosts

import (
	"context"

	"github.com/whisthq/whist/core-go/subscriptions"
)

type HostHandler interface {
	Initialize(region string) error
	SpinUpInstances(numInstances *int32, imageID string) (spunUp int, err error)
	SpinDownInstances(instanceIDs []string) error
	WaitForInstanceTermination(context.Context, subscriptions.Instance) error
}
