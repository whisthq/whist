package heroku

import (
	"os"
	"strings"
	"testing"

	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/utils"
)

func TestGetAppName(t *testing.T) {
	var herokuAppTests = []struct {
		environmentVar string
		want           string
	}{
		{"LOCALDEV", "whist-dev-scaling-service"},
		{"LOCALDEVWITHDB", "whist-dev-scaling-service"},
		{"DEV", "whist-dev-scaling-service"},
		{"STAGING", "whist-staging-scaling-service"},
		{"PROD", "whist-prod-scaling-service"},
		{"UNKNOWN", "whist-dev-scaling-service"},
	}

	for _, tt := range herokuAppTests {
		testname := utils.Sprintf("%s,%v", tt.environmentVar, tt.want)
		t.Run(testname, func(t *testing.T) {

			// Set the CI environment variable to the test environment
			os.Setenv("APP_ENV", tt.environmentVar)
			got := GetAppName()

			if got != tt.want {
				t.Errorf("got %v, want %v", got, tt.want)
			}
		})
	}
}

func TestGetHasuraName(t *testing.T) {
	var hasuraTests = []struct {
		environmentVar string
		want           string
	}{
		{"LOCALDEV", "whist-dev-hasura"},
		{"LOCALDEVWITHDB", "whist-dev-hasura"},
		{"DEV", "whist-dev-hasura"},
		{"STAGING", "whist-staging-hasura"},
		{"PROD", "whist-prod-hasura"},
		{"UNKNOWN", "whist-dev-hasura"},
	}

	for _, tt := range hasuraTests {
		testname := utils.Sprintf("%s,%v", tt.environmentVar, tt.want)
		t.Run(testname, func(t *testing.T) {

			// Set the CI environment variable to the test environment
			os.Setenv("APP_ENV", tt.environmentVar)
			got := GetHasuraName()

			if got != tt.want {
				t.Errorf("got %v, want %v", got, tt.want)
			}
		})
	}
}

func TestMain(m *testing.M) {
	// Set the GetAppEnvironment function before starting any tests.
	mockGetAppEnvironment()
	os.Exit(m.Run())
}

// mockGetAppEnvironment is used to mock the `GetAppEnvironment` function
// without the memoization. Doing this allows us to tests on all app environments.
func mockGetAppEnvironment() {
	metadata.GetAppEnvironment = func() metadata.AppEnvironment {
		env := strings.ToLower(os.Getenv("APP_ENV"))
		switch env {
		case "development", "dev":
			return metadata.EnvDev
		case "staging":
			return metadata.EnvStaging
		case "production", "prod":
			return metadata.EnvProd
		case "localdevwithdb", "localdev_with_db", "localdev_with_database":
			return metadata.EnvLocalDevWithDB
		default:
			return metadata.EnvLocalDev
		}
	}
}
