package subscriptions

import (
	"context"
	"net/http"

	graphql "github.com/hasura/go-graphql-client"
	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/utils"
	logger "github.com/whisthq/whist/backend/services/whistlogger"
)

// WhistGraphQLClient is an interface used to abstract the interactions with
// the official Hasura client.
type WhistGraphQLClient interface {
	Initialize(bool) error
	Query(context.Context, GraphQLQuery, map[string]interface{}) error
	Mutate(context.Context, GraphQLQuery, map[string]interface{}) error
}

// GraphQLClient implements WhistGraphQLClient and is exposed to be used
// by any other service that need to interact with the Hasura client.
type GraphQLClient struct {
	Hasura *graphql.Client
	Params HasuraParams
}

// withAdminSecretTransport is a custom http transport to authenticate all
// requests to Hasura using the admin secret.
type withAdminSecretTransport struct {
	AdminSecret string
}

// RoundTrip will inject the auth headers in each request to Hasura.
func (t *withAdminSecretTransport) RoundTrip(req *http.Request) (*http.Response, error) {
	req.Header.Add("content-type", "application/json")
	req.Header.Add("x-hasura-admin-secret", t.AdminSecret)

	return http.DefaultTransport.RoundTrip(req)
}

// Initialize creates the client. This function is respinsible from fetching the server
// information from Heroku.
func (wc *GraphQLClient) Initialize(useConfigDB bool) error {
	if !Enabled {
		logger.Infof("Running in app environment %s so not enabling GraphQL client code.", metadata.GetAppEnvironment())
		return nil
	}

	logger.Infof("Setting up GraphQL client...")

	var (
		params HasuraParams
		err    error
	)

	if useConfigDB {
		params, err = getWhistConfigHasuraParams()
		if err != nil {
			// Error obtaining the connection parameters, we stop and don't setup the client
			return utils.MakeError("error creating hasura client: %v", err)
		}
	} else {
		params, err = getWhistHasuraParams()
		if err != nil {
			// Error obtaining the connection parameters, we stop and don't setup the client
			return utils.MakeError("error creating hasura client: %v", err)
		}
	}
	wc.SetParams(params)

	// Create HTTP client for authenticating the GraphQL client
	httpClient := http.Client{
		Transport: &withAdminSecretTransport{
			AdminSecret: wc.GetParams().AccessKey,
		},
	}
	wc.Hasura = graphql.NewClient(wc.GetParams().URL, &httpClient)

	return nil
}

func (wc *GraphQLClient) GetParams() HasuraParams {
	return wc.Params
}

func (wc *GraphQLClient) SetParams(params HasuraParams) {
	wc.Params = params
}

// Query executes the given GraphQL query and assigns the returned values to
// the provided interface.
func (wc *GraphQLClient) Query(ctx context.Context, query GraphQLQuery, variables map[string]interface{}) error {
	dbCtx, cancel := context.WithCancel(ctx)
	defer cancel()

	return wc.Hasura.Query(dbCtx, query, variables)
}

// Mutate executes the given GraphQL mutation and writes to the database.
func (wc *GraphQLClient) Mutate(ctx context.Context, query GraphQLQuery, variables map[string]interface{}) error {
	dbCtx, cancel := context.WithCancel(ctx)
	defer cancel()

	return wc.Hasura.Mutate(dbCtx, query, variables)
}
