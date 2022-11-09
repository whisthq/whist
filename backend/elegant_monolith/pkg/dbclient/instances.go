package dbclient

import (
	"context"

	"github.com/whisthq/whist/backend/elegant_monolith/internal"
)

func (client Client) QueryInstance(ctx context.Context, id string) (internal.Instance, error) {
	var instance internal.Instance
	err := client.db.First(&instance).Error
	return instance, err
}

func (client Client) QueryInstancesWithCapacity(ctx context.Context, region string) ([]internal.Instance, error) {
	var instances []internal.Instance
	err := client.db.Where("region = ?", region).Find(&instances).Error
	return instances, err
}

// func QueryInstancesByStatusOnRegion(context.Context, string, string) ([]internal.Instance, error) {
// }

// func QueryInstancesByImage(context.Context, string) ([]internal.Instance, error) {
// }

// func InsertInstances(context.Context, []internal.Instance) (int, error) {
// }

// func UpdateInstance(context.Context, internal.Instance) (int, error) {
// }

// func DeleteInstance(context.Context, string) (int, error) {
// }
