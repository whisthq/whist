package dbclient

import (
	"context"

	"github.com/whisthq/whist/backend/elegant_monolith/internal"
)

func (client Client) QueryLatestImage(ctx context.Context, provider string, region string) (internal.Image, int, error) {
	var image internal.Image
	tx := client.db.Where("provider = ? AND region = ?", provider, region).Limit(1).Find(&image)
	return image, int(tx.RowsAffected), tx.Error
}
