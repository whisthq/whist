package dbclient

import (
	"context"

	"gorm.io/driver/postgres"
	"gorm.io/gorm"
)

type dbClient interface {
	QueryInstance(context.Context, string) ([]Instance, error)
	QueryInstanceWithCapacity(context.Context, string) ([]Instance, error)
	QueryInstancesByStatusOnRegion(context.Context, string, string) ([]Instance, error)
	QueryInstancesByImage(context.Context, string) ([]Instance, error)
	InsertInstances(context.Context, []Instance) (int, error)
	UpdateInstance(context.Context, Instance) (int, error)
	DeleteInstance(context.Context, string) (int, error)

	QueryImage(context.Context, string, string) ([]Image, error)
	QueryLatestImage(context.Context, string, string) (Image, error)
	InsertImages(context.Context, []Image) (int, error)
	UpdateImage(context.Context, Image) (int, error)

	QueryMandelbox(context.Context, string, string) ([]Mandelbox, error)
	QueryUserMandelboxes(context.Context, string) ([]Mandelbox, error)
	InsertMandelboxes(context.Context, []Mandelbox) (int, error)
	UpdateMandelbox(context.Context, Mandelbox) (int, error)
}

type Client struct {
	db *gorm.DB
}

func (client *Client) Init() {
	client.connect()
}

func (client *Client) connect() {
	dsn := "host=localhost user=gorm password=gorm dbname=gorm port=9920 sslmode=disable TimeZone=Asia/Shanghai"
	db, err := gorm.Open(postgres.Open(dsn), &gorm.Config{})
	client.db = db
}
