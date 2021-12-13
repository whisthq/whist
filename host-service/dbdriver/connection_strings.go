package dbdriver // import "github.com/whisthq/whist/host-service/dbdriver"

import (
	"github.com/whisthq/whist/core-go/metadata"
	"github.com/whisthq/whist/core-go/metadata/heroku"
	"github.com/whisthq/whist/core-go/utils"
)

// Whist database connection strings

const localDevDatabaseURL = "user=uap4ch2emueqo9 host=localhost port=9999 dbname=d9rf2k3vd6hvbm"

func getWhistDBConnString() (string, error) {
	if metadata.IsLocalEnv() {
		return localDevDatabaseURL, nil
	}

	config, err := heroku.GetConfig()
	if err != nil {
		return "", utils.MakeError("Couldn't get DB connection string: %s", err)
	}
	result, ok := config["DATABASE_URL"]
	if !ok {
		return "", utils.MakeError("Couldn't get DB connection string: couldn't find DATABASE_URL in Heroku environment.")
	}

	return result, nil
}
