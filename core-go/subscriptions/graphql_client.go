package subscriptions

import (
	"context"

	graphql "github.com/hasura/go-graphql-client"
	"github.com/whisthq/whist/core-go/utils"
	logger "github.com/whisthq/whist/core-go/whistlogger"
	"golang.org/x/oauth2"
)

// WhistGraphQLClient is an interface used to abstract the interactions with
// the official Hasura client.
type WhistGraphQLClient interface {
	Initialize() error
	Query(context.Context, GraphQLQuery, map[string]interface{}) error
	Mutate(context.Context, GraphQLQuery, map[string]interface{}) error
}

// GraphQLClient implements WhistGraphQLClient and is exposed to be used
// by any other service that need to interact with the Hasura client.
type GraphQLClient struct {
	Hasura *graphql.Client
	Params HasuraParams
}

// Initialize creates the client. This function is respinsible from fetching the server
// information from Heroku.
func (wc *GraphQLClient) Initialize() error {
	logger.Infof("Setting up GraphQL client...")

	params, err := getWhistHasuraParams()
	if err != nil {
		// Error obtaining the connection parameters, we stop and don't setup the client
		return utils.MakeError("error creating hasura client: %v", err)
	}

	wc.SetParams(params)

	// Create http client for authenticating the GraphQL client
	src := oauth2.StaticTokenSource(
		&oauth2.Token{AccessToken: wc.GetParams().AccessKey},
	)
	httpClient := oauth2.NewClient(context.Background(), src)

	wc.Hasura = graphql.NewClient(wc.GetParams().URL, httpClient)

	return nil
}

func (wc *GraphQLClient) GetParams() HasuraParams {
	return wc.Params
}

func (wc *GraphQLClient) SetParams(params HasuraParams) {
	wc.Params = params
}

// Query executes the given GraphQL query and assigns the reeturned values to
// the provided interface.
func (wc *GraphQLClient) Query(ctx context.Context, query GraphQLQuery, variables map[string]interface{}) error {
	err := wc.Hasura.Query(ctx, &query, variables)

	if err != nil {
		return utils.MakeError("failed to get GraphQL query %v with variables %v. Error: %v", query, variables, err)
	}

	return nil
}

// Mutate executes the given GraphQL mutation and writes to the database.
func (wc *GraphQLClient) Mutate(ctx context.Context, query GraphQLQuery, variables map[string]interface{}) error {
	err := wc.Hasura.Mutate(ctx, &query, variables)

	if err != nil {
		return utils.MakeError("failed to execute GraphQL mutation %v with variables %v. Error: %v", query, variables, err)
	}

	logger.Infof("Successfully wrote %v to database", variables)

	return nil
}
