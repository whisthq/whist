package dbclient

import (
	"context"

	"github.com/whisthq/whist/backend/elegant_monolith/internal"
	"github.com/whisthq/whist/backend/elegant_monolith/pkg/logger"

	"gorm.io/driver/postgres"
	"gorm.io/gorm"
)

type DBClient interface {
	QueryInstance(ctx context.Context, instanceID string) (internal.Instance, int, error)
	QueryInstancesWithCapacity(ctx context.Context, region string) ([]internal.Instance, int, error)
	QueryInstancesByStatusOnRegion(ctx context.Context, status string, region string) ([]internal.Instance, int, error)
	// QueryInstancesByImage(context.Context, string) ([]internal.Instance, error)
	InsertInstances(ctx context.Context, instances []internal.Instance) (int, error)
	UpdateInstance(ctx context.Context, update internal.Instance) (int, error)
	// DeleteInstance(context.Context, string) (int, error)

	// QueryImage(context.Context, string, string) ([]Image, error)
	QueryLatestImage(ctx context.Context, provider string, region string) (internal.Image, int, error)
	// InsertImages(context.Context, []Image) (int, error)
	// UpdateImage(context.Context, Image) (int, error)

	QueryMandelbox(ctx context.Context, instanceID string, status string) (internal.Mandelbox, int, error)
	QueryUserMandelboxes(ctx context.Context, userID string) ([]internal.Mandelbox, int, error)
	InsertMandelboxes(ctx context.Context, mandelboxes []internal.Mandelbox) (int, error)
	UpdateMandelbox(ctx context.Context, update internal.Mandelbox) (int, error)
}

type Client struct {
	db *gorm.DB
}

func NewConnection() DBClient {
	cl := &Client{}
	db, err := connect()
	if err != nil {
		logger.Infof("error starting db: %s", err)
		return nil
	}
	logger.Infof("Successfully connected to db")
	cl.db = db
	return cl
}

func connect() (*gorm.DB, error) {
	dsn := "host=localhost user=postgres password=whistpass dbname=postgres port=5432"
	db, err := gorm.Open(postgres.Open(dsn), &gorm.Config{})
	if err != nil {
		return nil, err
	}
	return db, nil
}
