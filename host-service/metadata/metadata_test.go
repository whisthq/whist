package metadata

import (
	"os"
	"strings"
	"testing"

	"github.com/fractal/fractal/host-service/utils"
)

var environmentTests = []struct {
	environmentVar string
	want           AppEnvironment
}{
	{"LOCALDEV", "LOCALDEV"},
	{"LOCALDEVWITHDB", "LOCALDEVWITHDB"},
	{"DEV", "DEV"},
	{"STAGING", "STAGING"},
	{"PROD", "PROD"},
	{"UNKNOWN", "LOCALDEV"},
}

func TestGetAppEnvironment(t *testing.T) {
	for _, tt := range environmentTests {
		testname := utils.Sprintf("%s,%s", tt.environmentVar, tt.want)
		t.Run(testname, func(t *testing.T) {

			// Set the APP_ENV environment variable to the test environment
			os.Setenv("APP_ENV", tt.environmentVar)
			got := GetAppEnvironment()

			if got != tt.want {
				t.Errorf("got %s, want %s", got, tt.want)
			}
		})
	}
}
func TestIsLocalEnv(t *testing.T) {
	for _, tt := range environmentTests {
		want := tt.want == EnvLocalDev || tt.want == EnvLocalDevWithDB

		testname := utils.Sprintf("%s,%v", tt.environmentVar, want)
		t.Run(testname, func(t *testing.T) {

			// Set the APP_ENV environment variable to the test environment
			os.Setenv("APP_ENV", tt.environmentVar)
			got := IsLocalEnv()

			if got != want {
				t.Errorf("got %v, want %v", got, want)
			}
		})
	}
}
func TestIsLocalEnvWithoutDB(t *testing.T) {
	for _, tt := range environmentTests {
		want := tt.want == EnvLocalDev

		testname := utils.Sprintf("%s,%v", tt.environmentVar, want)
		t.Run(testname, func(t *testing.T) {

			// Set the APP_ENV environment variable to the test environment
			os.Setenv("APP_ENV", tt.environmentVar)
			got := IsLocalEnvWithoutDB()

			if got != want {
				t.Errorf("got %v, want %v", got, want)
			}
		})
	}
}

func TestGetAppEnvironmentLowercase(t *testing.T) {
	var environmentTests = []struct {
		environmentVar string
		want           string
	}{
		{"LOCALDEV", "localdev"},
		{"LOCALDEVWITHDB", "localdevwithdb"},
		{"DEV", "dev"},
		{"STAGING", "staging"},
		{"PROD", "prod"},
		{"UNKNOWN", "localdev"},
	}

	for _, tt := range environmentTests {
		testname := utils.Sprintf("%s,%s", tt.environmentVar, tt.want)
		t.Run(testname, func(t *testing.T) {

			// Set the APP_ENV environment variable to the test environment
			os.Setenv("APP_ENV", tt.environmentVar)
			got := GetAppEnvironmentLowercase()

			if got != tt.want {
				t.Errorf("got %v, want %v", got, tt.want)
			}
		})
	}

}
func TestIsRunningInCI(t *testing.T) {
	var CITests = []struct {
		environmentVar string
		want           bool
	}{
		{"1", true},
		{"yes", true},
		{"true", true},
		{"on", true},
		{"yep", true},
		{"0", false},
		{"no", false},
		{"false", false},
		{"off", false},
		{"nope", false},
		{"unknown", false},
	}

	for _, tt := range CITests {
		testname := utils.Sprintf("%s,%v", tt.environmentVar, tt.want)
		t.Run(testname, func(t *testing.T) {

			// Set the CI environment variable to the test environment
			os.Setenv("CI", tt.environmentVar)
			got := IsRunningInCI()

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
	GetAppEnvironment = func() AppEnvironment {
		env := strings.ToLower(os.Getenv("APP_ENV"))
		switch env {
		case "development", "dev":
			return EnvDev
		case "staging":
			return EnvStaging
		case "production", "prod":
			return EnvProd
		case "localdevwithdb", "localdev_with_db", "localdev_with_database":
			return EnvLocalDevWithDB
		default:
			return EnvLocalDev
		}
	}
}
