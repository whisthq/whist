package subscriptions // import "github.com/whisthq/whist/backend/services/host-service/subscriptions"

import (
	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/metadata/heroku"
	"github.com/whisthq/whist/backend/services/utils"
)

// Whist database connection strings

const localHasuraURL = "http://localhost:8080/v1/graphql"
const localHasuraConfigURL = "http://localhost:8082/v1/graphql"

// getWhistHasuraParams obtains and returns the heroku parameters
// from the metadata package that are necessary to initialize the client.
func getWhistHasuraParams() (HasuraParams, error) {
	if metadata.IsLocalEnv() {
		return HasuraParams{
			URL:       localHasuraURL,
			AccessKey: "hasura",
		}, nil
	}

	url, err := heroku.GetHasuraURL()
	if err != nil {
		return HasuraParams{}, utils.MakeError("Couldn't get Hasura connection URL: %s", err)
	}
	config, err := heroku.GetHasuraConfig()
	if err != nil {
		return HasuraParams{}, utils.MakeError("Couldn't get Hasura config: %s", err)
	}
	result, ok := config["HASURA_GRAPHQL_ACCESS_KEY"]
	if !ok {
		return HasuraParams{}, utils.MakeError("Couldn't get Hasura connection URL: couldn't find HASURA_GRAPHQL_ACCESS_KEY in Heroku environment.")
	}

	params := HasuraParams{
		URL:       url,
		AccessKey: result,
	}

	return params, nil
}

// getWhistHasuraParams obtains and returns the heroku parameters from
// the metadata package that are necessary to initialize the config client.
func getWhistConfigHasuraParams() (HasuraParams, error) {
	if metadata.IsLocalEnv() {
		return HasuraParams{
			URL:       localHasuraConfigURL,
			AccessKey: "hasura",
		}, nil
	}

	url, err := heroku.GetConfigHasuraURL()
	if err != nil {
		return HasuraParams{}, utils.MakeError("Couldn't get config Hasura connection URL: %s", err)
	}
	config, err := heroku.GetConfigHasuraVars()
	if err != nil {
		return HasuraParams{}, utils.MakeError("Couldn't get config Hasura : %s", err)
	}
	result, ok := config["HASURA_GRAPHQL_ACCESS_KEY"]
	if !ok {
		return HasuraParams{}, utils.MakeError("Couldn't get config Hasura connection URL: couldn't find HASURA_GRAPHQL_ACCESS_KEY in Heroku environment.")
	}

	params := HasuraParams{
		URL:       url,
		AccessKey: result,
	}

	return params, nil
}
