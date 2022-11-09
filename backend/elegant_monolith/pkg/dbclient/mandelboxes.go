package dbclient

import (
	"context"

	"github.com/whisthq/whist/backend/elegant_monolith/internal"
)

func (client Client) QueryUserMandelboxes(ctx context.Context, userID string) ([]internal.Mandelbox, error) {
	var mandelboxes []internal.Mandelbox
	err := client.db.Where("user_id = ?", userID).Find(&mandelboxes).Error
	return mandelboxes, err
}
