package dbclient

import (
	"context"

	"github.com/whisthq/whist/backend/services/subscriptions"
)

// WhistDBClient is an interface that abstracts all interactions with
// the database, it includes query, insert, update and delete methos for
// the `whist.instances` and `whist.images` table. By abstracting the methods
// we can easily test and mock the scaling algorithm actions.
type WhistDBClient interface {
	QueryInstance(context.Context, subscriptions.WhistGraphQLClient, string) (subscriptions.WhistInstances, error)
	QueryInstancesByStatusOnRegion(context.Context, subscriptions.WhistGraphQLClient, string, string) (subscriptions.WhistInstances, error)
	QueryInstancesByImage(context.Context, subscriptions.WhistGraphQLClient, string) (subscriptions.WhistInstances, error)
	InsertInstances(context.Context, subscriptions.WhistGraphQLClient, []subscriptions.Instance) (int, error)
	UpdateInstance(context.Context, subscriptions.WhistGraphQLClient, map[string]interface{}) (int, error)
	DeleteInstance(context.Context, subscriptions.WhistGraphQLClient, string) (int, error)
	QueryImage(context.Context, subscriptions.WhistGraphQLClient, string, string) (subscriptions.WhistImages, error)
	InsertImages(context.Context, subscriptions.WhistGraphQLClient, []subscriptions.Image) (int, error)
	UpdateImage(context.Context, subscriptions.WhistGraphQLClient, subscriptions.Image) (int, error)
}

// DBClient implements `WhistDBClient`, it is the default database
// used on the default scaling algorithm.
type DBClient struct{}
