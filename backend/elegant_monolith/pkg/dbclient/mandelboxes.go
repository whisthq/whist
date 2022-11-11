package dbclient

import (
	"context"

	"github.com/whisthq/whist/backend/elegant_monolith/internal"
)

func (client Client) InsertMandelboxes(ctx context.Context, mandelboxes []internal.Mandelbox) (int, error) {
	tx := client.db.Create(&mandelboxes)
	return int(tx.RowsAffected), tx.Error
}

func (client Client) QueryMandelbox(ctx context.Context, instanceID string, status string) (internal.Mandelbox, int, error) {
	var mandelbox internal.Mandelbox
	tx := client.db.Where("instance_id = ? AND status = ?", instanceID, status).Limit(1).Find(&mandelbox)
	return mandelbox, int(tx.RowsAffected), tx.Error
}

func (client Client) QueryUserMandelboxes(ctx context.Context, userID string) ([]internal.Mandelbox, int, error) {
	var mandelboxes []internal.Mandelbox
	tx := client.db.Where("user_id = ?", userID).Find(&mandelboxes)
	return mandelboxes, int(tx.RowsAffected), tx.Error
}

func (client Client) UpdateMandelbox(ctx context.Context, update internal.Mandelbox) (int, error) {
	tx := client.db.Save(&update)
	return int(tx.RowsAffected), tx.Error
}
