package hosts

import (
	"context"
	"time"

	"github.com/whisthq/whist/backend/services/subscriptions"
)

type GCPHost struct{}

func (gc *GCPHost) Initialize(region string) error {
	return nil
}

// Return the provider's name. This access method is necessary (as opposed to using a constant or struct field)
// so the scaling algorithm can abstract any provider-specific logic.
func (gc *GCPHost) GetProvider() string {
	return PROVIDER_NAME
}

// Return the default instance type. This access method is necessary (as opposed to using a constant or struct field)
// so the scaling algorithm can abstract any provider-specific logic.
func (gc *GCPHost) GetInstanceType() string {
	// TODO: Once support for different instance types is added, decide
	// which kinnd of instance type to return.
	return INSTANCE_TYPE
}

func (gc *GCPHost) SpinUpInstances(scalingCtx context.Context, numInstances int32, maxWaitTimeReady time.Duration, image subscriptions.Image) (createdInstances []subscriptions.Instance, err error) {
	return []subscriptions.Instance{}, nil
}

func (gc *GCPHost) SpinDownInstances(scalingCtx context.Context, instanceIDs []string) error {
	return nil
}

func (gc *GCPHost) WaitForInstanceTermination(context.Context, time.Duration, []string) error {
	return nil
}

func (gc *GCPHost) WaitForInstanceReady(context.Context, time.Duration, []string) error {
	return nil
}
