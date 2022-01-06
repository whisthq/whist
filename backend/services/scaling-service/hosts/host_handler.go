package hosts

import (
	"context"

	"github.com/whisthq/whist/backend/services/subscriptions"
)

type HostHandler interface {
	Initialize(region string) error
	SpinUpInstances(numInstances int32, imageID string) (createdInstances []subscriptions.Host, err error)
	SpinDownInstances(instanceIDs []string) (terminatedInstances []subscriptions.Host, err error)
	WaitForInstanceTermination(context.Context, subscriptions.Host) error
	WaitForInstanceReady(context.Context, subscriptions.Host) error
	RetryInstanceSpinUp()
}
