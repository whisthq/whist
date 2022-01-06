package hosts

import (
	"context"

	"github.com/whisthq/whist/backend/services/subscriptions"
)

type HostHandler interface {
	Initialize(region string) error
	SpinUpInstances(scalingCtx context.Context, numInstances int32, imageID string) (createdInstances []subscriptions.Instance, err error)
	SpinDownInstances(scalingCtx context.Context, instanceIDs []string) (terminatedInstances []subscriptions.Instance, err error)
	WaitForInstanceTermination(context.Context, []string) error
	WaitForInstanceReady(context.Context, []string) error
}
