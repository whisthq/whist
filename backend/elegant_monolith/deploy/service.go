package deploy

import (
	"context"
	"github.com/whisthq/whist/backend/elegant_monolith/internal"
)

type DeployService interface {
	internal.Service
	UpgradeImage(ctx context.Context, imageID string) (err error)
	SwapoverImages(ctx context.Context, version string) (err error)
}
