// Copyright (c) 2021-2022 Whist Technologies, Inc.

/*
Package dbclient abstracts all interactions with the database for any
scaling algorithm to use. It defines an interface so any consumers of
this package can perform query, update and delete operations without
having to use the Hasura client directy.
*/

package dbclient

import (
	"context"

	"github.com/whisthq/whist/backend/services/subscriptions"
)

// WhistDBClient is an interface that abstracts all interactions with
// the database, it includes query, insert, update and delete methods for
// the `whist.instances`, `whist.images` and `whist.mandelboxes` tables. By
// abstracting the methods we can easily test and mock the scaling algorithm actions.
type WhistDBClient interface {
	QueryInstance(context.Context, subscriptions.WhistGraphQLClient, string) ([]subscriptions.Instance, error)
	QueryInstanceWithCapacity(context.Context, subscriptions.WhistGraphQLClient, string) ([]subscriptions.Instance, error)
	QueryInstancesByStatusOnRegion(context.Context, subscriptions.WhistGraphQLClient, string, string) ([]subscriptions.Instance, error)
	QueryInstancesByImage(context.Context, subscriptions.WhistGraphQLClient, string) ([]subscriptions.Instance, error)
	InsertInstances(context.Context, subscriptions.WhistGraphQLClient, []subscriptions.Instance) (int, error)
	UpdateInstance(context.Context, subscriptions.WhistGraphQLClient, subscriptions.Instance) (int, error)
	DeleteInstance(context.Context, subscriptions.WhistGraphQLClient, string) (int, error)
	QueryImage(context.Context, subscriptions.WhistGraphQLClient, string, string) ([]subscriptions.Image, error)
	InsertImages(context.Context, subscriptions.WhistGraphQLClient, []subscriptions.Image) (int, error)
	UpdateImage(context.Context, subscriptions.WhistGraphQLClient, subscriptions.Image) (int, error)
	QueryMandelbox(context.Context, subscriptions.WhistGraphQLClient, string, string) ([]subscriptions.Mandelbox, error)
	UpdateMandelbox(context.Context, subscriptions.WhistGraphQLClient, subscriptions.Mandelbox) (int, error)
}

// DBClient implements `WhistDBClient`, it is the default database
// used on the default scaling algorithm.
type DBClient struct{}
