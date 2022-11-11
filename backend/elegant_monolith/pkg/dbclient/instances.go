package dbclient

import (
	"context"

	"github.com/whisthq/whist/backend/elegant_monolith/internal"
)

func (client Client) InsertInstances(ctx context.Context, instances []internal.Instance) (int, error) {
	tx := client.db.Create(&instances)
	return int(tx.RowsAffected), tx.Error
}

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

func (client Client) QueryInstancesByStatusOnRegion(ctx context.Context, status string, region string) ([]internal.Instance, int, error) {
	var instances []internal.Instance
	tx := client.db.Where("status = ? AND region = ?", status, region).Find(&instances)
	return instances, int(tx.RowsAffected), tx.Error
}

// func QueryInstancesByImage(context.Context, string) ([]internal.Instance, error) {
// }

func (client Client) UpdateInstance(ctx context.Context, instance internal.Instance) (int, error) {
	tx := client.db.Save(&instance)
	return int(tx.RowsAffected), tx.Error
}

// func DeleteInstance(context.Context, string) (int, error) {
// }
