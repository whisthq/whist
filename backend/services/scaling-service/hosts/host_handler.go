package hosts

import (
	"context"
	"time"

	"github.com/whisthq/whist/backend/services/subscriptions"
)

// HostHandler defines the necessary methods for performing scaling actions. There should be a different
// implementation for each cloud service provider, which will interact directly with the provider's API.
type HostHandler interface {
	Initialize(region string) error
	SpinUpInstances(scalingCtx context.Context, numInstances int32, maxWaitTimeReady time.Duration, image subscriptions.Image) (createdInstances []subscriptions.Instance, err error)
	SpinDownInstances(scalingCtx context.Context, instanceIDs []string) (terminatedInstances []subscriptions.Instance, err error)
	WaitForInstanceTermination(context.Context, time.Duration, []string) error
	WaitForInstanceReady(context.Context, time.Duration, []string) error
}
