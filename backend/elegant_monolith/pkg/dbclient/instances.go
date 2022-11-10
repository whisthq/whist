package dbclient

import (
	"context"

	"github.com/whisthq/whist/backend/elegant_monolith/internal"
)

func (client Client) QueryInstance(ctx context.Context, id string) (internal.Instance, int, error) {
	var instance internal.Instance
	tx := client.db.Limit(1).Find(&instance)
	return instance, int(tx.RowsAffected), tx.Error
}

func (client Client) QueryInstancesWithCapacity(ctx context.Context, region string) ([]internal.Instance, int, error) {
	var instances []internal.Instance
	tx := client.db.Where("region = ?", region).Find(&instances)
	return instances, int(tx.RowsAffected), tx.Error
}

// func QueryInstancesByStatusOnRegion(context.Context, string, string) ([]internal.Instance, error) {
// }

// func QueryInstancesByImage(context.Context, string) ([]internal.Instance, error) {
// }

// func InsertInstances(context.Context, []internal.Instance) (int, error) {
// }

func (client Client) UpdateInstance(ctx context.Context, instance internal.Instance) (int, error) {
	tx := client.db.Save(&instance)
	return int(tx.RowsAffected), tx.Error
}

// func DeleteInstance(context.Context, string) (int, error) {
// }
