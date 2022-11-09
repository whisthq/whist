package dbclient

import (
	"context"
	"log"

	"github.com/whisthq/whist/backend/elegant_monolith/internal"
	"gorm.io/driver/postgres"
	"gorm.io/gorm"
)

type DBClient interface {
	QueryInstance(ctx context.Context, instanceID string) (internal.Instance, error)
	QueryInstancesWithCapacity(ctx context.Context, region string) ([]internal.Instance, error)
	// QueryInstancesByStatusOnRegion(context.Context, string, string) ([]internal.Instance, error)
	// QueryInstancesByImage(context.Context, string) ([]internal.Instance, error)
	// InsertInstances(context.Context, []internal.Instance) (int, error)
	// UpdateInstance(context.Context, internal.Instance) (int, error)
	// DeleteInstance(context.Context, string) (int, error)

	// QueryImage(context.Context, string, string) ([]Image, error)
	QueryLatestImage(ctx context.Context, provider string, region string) (internal.Image, error)
	// InsertImages(context.Context, []Image) (int, error)
	// UpdateImage(context.Context, Image) (int, error)

	QueryUserMandelboxes(ctx context.Context, userID string) ([]internal.Mandelbox, error)
	// InsertMandelboxes(context.Context, []Mandelbox) (int, error)
	// UpdateMandelbox(context.Context, Mandelbox) (int, error)
}

type Client struct {
	db *gorm.DB
}

func NewConnection() DBClient {
	cl := &Client{}
	db, err := connect()
	if err != nil {
		log.Printf("error starting db: %s", err)
		return nil
	}

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
