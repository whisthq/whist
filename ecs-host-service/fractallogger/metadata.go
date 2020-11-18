package fractallogger

import (
	"bufio"
	"fmt"
	"io/ioutil"
	"net/http"
	"os"
	"runtime"
	"strings"
	"time"
)

// Variable for hash of last Git commit --- filled in by linker
var gitCommit string

// GetGitCommit returns the git commit hash of this build.
func GetGitCommit() string {
	return gitCommit
}

// An EnvironmentType represents either dev, staging, or prod
type EnvironmentType string

// Constants for whether we are running in dev, staging, or prod
const (
	EnvDev     EnvironmentType = "DEV"
	EnvStaging EnvironmentType = "STAGING"
	EnvProd    EnvironmentType = "PROD"
)

// GetAppEnvironment returns the EnvironmentType of the current instance
func GetAppEnvironment() EnvironmentType {
	env := os.Getenv("APP_ENV")
	switch env {
	case "development", "dev", "DEVELOPMENT", "DEV":
		return EnvDev
	case "staging", "STAGING":
		return EnvStaging
	case "production", "prod", "PRODUCTION", "PROD":
		return EnvProd
	default:
		return EnvDev
	}
}

// The following functions are useful for getting system information that we
// will pass to the Webserver

// GetNumLogicalCPUs returns the number of logical CPUs on the host
func GetNumLogicalCPUs() (string, error) {
	return fmt.Sprint(runtime.NumCPU()), nil
}

// GetTotalMemoryInKB returns the total memory of the host in KB. This function
// can be memoized because the total memory will not change over time.
var GetTotalMemoryInKB = memoizeString(func() (string, error) {
	return calculateMemoryStatInKB("MemTotal")
})

// GetFreeMemoryInKB returns the amount of currently free memory in the host in
// KB. This is a lower bound for how much more memory we can theoretically use,
// since some cached files can be evicted to free up some more memory.
var GetFreeMemoryInKB = func() (string, error) {
	return calculateMemoryStatInKB("MemFree")
}

// GetAvailableMemoryInKB returns the amount of currently available memory in
// the host in KB. This is a hard upper bound for how much more memory we can
// use before we start swapping or crash. We should stay well below this number
// because performance will suffer long before we get here.
var GetAvailableMemoryInKB = func() (string, error) {
	return calculateMemoryStatInKB("MemAvailable")
}

// This is a helper function for us to read memory info from the host
func calculateMemoryStatInKB(field string) (string, error) {
	const meminfoPath = "/proc/meminfo"
	file, err := os.Open(meminfoPath)
	if err != nil {
		return "", MakeError("Unable to open file %s: Error: %v", meminfoPath, err)
	}
	defer file.Close()

	scanner := bufio.NewScanner(file)
	for scanner.Scan() {
		line := strings.TrimSpace(scanner.Text())
		if strings.HasPrefix(line, field) {
			line = strings.TrimPrefix(line, field+":")
			line = strings.TrimSpace(line)
			line = strings.TrimSuffix(line, "kB")
			line = strings.TrimSpace(line)
			return line, nil
		}
	}
	if err := scanner.Err(); err != nil {
		return "", MakeError("Error scanning %s: %v", meminfoPath, err)
	}
	return "", MakeError("Could not find field %s in file %s", field, meminfoPath)
}

// All of these have signature (func() (string, error))

// GetAwsAmiID returns the AMI ID of the current EC2 instance
var GetAwsAmiID = memoizeString(generateAWSMetadataRetriever("ami-id"))

// GetAwsAmiLaunchIndex returns the AWS Launch Index
var GetAwsAmiLaunchIndex = memoizeString(generateAWSMetadataRetriever("ami-launch-index"))

// GetAwsInstanceID returns the Instance ID of the current EC2 instance
var GetAwsInstanceID = memoizeString(generateAWSMetadataRetriever("instance-id"))

// GetAwsInstanceType returns the type of the current EC2 instance (e.g. g3s.xlarge)
var GetAwsInstanceType = memoizeString(generateAWSMetadataRetriever("instance-type"))

// GetAwsPlacementRegion returns the placement region of the current EC2 instance
var GetAwsPlacementRegion = memoizeString(generateAWSMetadataRetriever("placement/region"))

// GetAwsPublicIpv4 returns the public IPv4 address of the current EC2 instance
var GetAwsPublicIpv4 = memoizeString(generateAWSMetadataRetriever("public-ipv4"))

// This helper function lets us memoize a function of type (func() string,
// error) into another function of type (func() string, error). We do not cache
// the results from a failed call (non-nil error). We use this so that we only
// have to query the AWS endpoint successfully once to get information for our
// startup, shutdown, and heartbeat messages. This architecture is compelling
// because of reports online that the AWS endpoint to query this information
// has transient failures. We want to minimize the impact of these transient
// failures on our application --- with this approach, host startup will be
// affected by these failures, but transient failures should not affect already
// running hosts.
func memoizeString(f func() (string, error)) func() (string, error) {
	var cached bool = false
	var cachedResult string
	return func() (string, error) {
		if cached {
			return cachedResult, nil
		}
		result, err := f()
		if err != nil {
			return result, err
		}
		cachedResult = result
		cached = true
		return cachedResult, nil
	}
}

// This helper function generates functions that retrieve metadata from the
// internal AWS endpoint. We use this together with memoizeString() to expose
// memoized functions that query information about the host from AWS.
func generateAWSMetadataRetriever(path string) func() (string, error) {
	const AWSendpointBase = "http://169.254.169.254/latest/meta-data/"
	httpClient := http.Client{
		Timeout: 1 * time.Second,
	}

	url := AWSendpointBase + path
	return func() (string, error) {
		resp, err := httpClient.Get(url)
		if err != nil {
			return "", MakeError("Error retrieving data from URL %s: %v. Is the service running on an AWS EC2 instance?", url, err)
		}
		defer resp.Body.Close()

		body, err := ioutil.ReadAll(resp.Body)
		if err != nil {
			return string(body), MakeError("Error reading response body from URL %s: %v", url, err)
		}
		if resp.StatusCode != 200 {
			return string(body), MakeError("Got non-200 response from URL %s: %s", url, resp.Status)
		}
		return string(body), nil
	}
}
