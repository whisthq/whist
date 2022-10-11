package dbdriver // import "github.com/whisthq/whist/backend/services/host-service/dbdriver"

import (
	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/metadata/heroku"
	"github.com/whisthq/whist/backend/services/utils"
)

// Whist database connection strings

const localDevDatabaseURL = "user=postgres host=localhost port=5432 dbname=postgres password=whistpass"

func getWhistDBConnString() (string, error) {
	if metadata.IsLocalEnv() {
		return localDevDatabaseURL, nil
	}

	config, err := heroku.GetConfig()
	if err != nil {
		return "", utils.MakeError("couldn't get DB connection string: %s", err)
	}
	result, ok := config["DATABASE_URL"]
	if !ok {
		return "", utils.MakeError("couldn't get DB connection string: couldn't find DATABASE_URL in Heroku environment")
	}

	return result, nil
}
