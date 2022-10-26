// types.go

package internal

import (
	"context"

	"github.com/hasura/go-graphql-client"
)

type Querier interface {
	Query(context.Context, interface{}, map[string]interface{}, ...graphql.Option) error
}
