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

func (gc *GCPHost) GetProvider() string {
	return "GCP"
}

func (gc *GCPHost) GetInstanceType() string {
	return ""
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
