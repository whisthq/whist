package dbclient

import (
	"context"

	"github.com/whisthq/whist/backend/elegant_monolith/internal"
)

func (client Client) QueryLatestImage(ctx context.Context, provider string, region string) (internal.Image, error) {
	var image internal.Image
	err := client.db.Where("provider = ? AND region = ?", provider, region).First(&image).Error
	return image, err
}
