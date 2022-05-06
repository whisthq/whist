// Copyright (c) 2021-2022 Whist Technologies, Inc.

/*
Package hosts defines an interface that abstracts any actions that call a cloud
provider API to start, stop and wait for instances that run Whist. Any implementation
for a different cloud provider should be defined as a subpackage athat implements the
main HostHandler interface.
*/

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
