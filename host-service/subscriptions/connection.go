package subscriptions // import "github.com/fractal/fractal/host-service/subscriptions"

import (
	"github.com/fractal/fractal/host-service/metadata"
	"github.com/fractal/fractal/host-service/metadata/heroku"
	"github.com/fractal/fractal/host-service/utils"
)

// Fractal database connection strings

const localHasuraURL = "http://localhost:8080/v1/graphql"

// getFractalHasuraParams obtains and returns the heroku parameters
// from the metadata package that are necessary to initialize the client.
func getFractalHasuraParams() (interface{}, error) {
	if metadata.IsLocalEnv() {
		return HasuraParams{
			URL:       localHasuraURL,
			AccessKey: "hasura",
		}, nil
	}

	url, err := heroku.GetHasuraURL()
	if err != nil {
		return "", utils.MakeError("Couldn't get Hasura connection URL: %s", err)
	}
	config, err := heroku.GetHasuraConfig()
	if err != nil {
		return "", utils.MakeError("Couldn't get Hasura config: %s", err)
	}
	result, ok := config["HASURA_GRAPHQL-ACCESS-KEY"]
	if !ok {
		return "", utils.MakeError("Couldn't get Hasura connection URL: couldn't find HASURA_GRAPHQL-ACCESS-KEY in Heroku environment.")
	}

	params := HasuraParams{
		URL:       url,
		AccessKey: result,
	}

	return params, nil
}
