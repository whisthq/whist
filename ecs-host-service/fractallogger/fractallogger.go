package fractallogger

import (
	"bufio"
	"fmt"
	"io/ioutil"
	"log"
	"net/http"
	"os"
	"runtime"
	"runtime/debug"
	"strings"
	"time"

	"github.com/getsentry/sentry-go"
)

// Create an error from format string and args
func MakeError(format string, v ...interface{}) error {
	return fmt.Errorf(format, v...)
}

// Create string from format string and args
func Sprintf(format string, v ...interface{}) string {
	return fmt.Sprintf(format, v...)
}

// Log an error and send it to sentry.
func Error(err error) {
	errstr := fmt.Sprintf("ERROR: %v", err)
	log.Println(errstr)
	sentry.CaptureException(err)
}

// Panic on an error. We do not send it to Sentry (to save on Sentry logging
// costs), since we do that when we recover from the panic in
// shutdownHostService(). Note that there are two types of panics that we are
// unable to send to Sentry using our current approach: panics in a goroutine
// that didn't `defer shutdownHostService()` as soon as it started, and panics
// caused by incorrect initialization of Sentry in the first place.
func Panic(err error) {
	log.Panic(err)
}

// Log some info and send it to sentry as well
func Info(format string, v ...interface{}) {
	str := fmt.Sprintf(format, v...)
	log.Print(str)
	sentry.CaptureMessage(str)
}

// We also create a pair of functions that respect printf syntax, i.e. take in
// a format string and arguments, for convenience
func Errorf(format string, v ...interface{}) {
	Error(MakeError(format, v...))
}

func Panicf(format string, v ...interface{}) {
	Panic(MakeError(format, v...))
}

// For consistency, we also include a variant for Info even though it already
// takes in a format string and arguments.
func Infof(format string, v ...interface{}) {
	Info(format, v...)
}

func PrintStackTrace() {
	Info("Printing stack trace: ")
	debug.PrintStack()
}

func InitializeSentry() error {
	err := sentry.Init(sentry.ClientOptions{
		Dsn:   "https://5f2dd9674e2b4546b52c205d7382ac90@o400459.ingest.sentry.io/5461239",
		Debug: true,
	})
	if err != nil {
		return MakeError("Error calling Sentry.init: %v", err)
	}

	// Configure Sentry's scope with some instance-specific information
	sentry.ConfigureScope(func(scope *sentry.Scope) {
		// This function looks repetitive, but we can't refactor its functionality
		// into a separate function because we want to defer sending the errors
		// about being unable to set Sentry tags until after we have set all the
		// ones we can.
		if val, err := GetAwsAmiId(); err != nil {
			defer Errorf("Unable to set Sentry tag aws.ami-id: %v", err)
		} else {
			scope.SetTag("aws.ami-id", val)
			log.Printf("Set sentry tag aws.ami-id: %s", val)
		}

		if val, err := GetAwsAmiLaunchIndex(); err != nil {
			defer Errorf("Unable to set Sentry tag aws.ami-launch-index: %v", err)
		} else {
			scope.SetTag("aws.ami-launch-index", val)
			log.Printf("Set sentry tag aws.ami-launch-index: %s", val)
		}

		if val, err := GetAwsInstanceId(); err != nil {
			defer Errorf("Unable to set Sentry tag aws.instance-id: %v", err)
		} else {
			scope.SetTag("aws.instance-id", val)
			log.Printf("Set sentry tag aws.instance-id: %s", val)
		}

		if val, err := GetAwsInstanceType(); err != nil {
			defer Errorf("Unable to set Sentry tag aws.instance-type: %v", err)
		} else {
			scope.SetTag("aws.instance-type", val)
			log.Printf("Set sentry tag aws.instance-type: %s", val)
		}

		if val, err := GetAwsPlacementRegion(); err != nil {
			defer Errorf("Unable to set Sentry tag aws.placement-region: %v", err)
		} else {
			scope.SetTag("aws.placement-region", val)
			log.Printf("Set sentry tag aws.placement-region: %s", val)
		}

		if val, err := GetAwsPublicIpv4(); err != nil {
			defer Errorf("Unable to set Sentry tag aws.public-ipv4: %v", err)
		} else {
			scope.SetTag("aws.public-ipv4", val)
			log.Printf("Set sentry tag aws.public-ipv4: %s", val)
		}
	})
	return nil
}

func FlushSentry() {
	sentry.Flush(5 * time.Second)
}

// The following functions are useful for getting system information that we
// will pass to the Webserver

// Get the number of logical CPUs on the host
func GetNumLogicalCPUs() (string, error) {
	return fmt.Sprint(runtime.NumCPU()), nil
}

// Get the total memory of the host in KB. This function can be memoized
// because the total memory will not change over time.
var GetTotalMemoryInKB = memoizeString(func() (string, error) {
	return calculateMemoryStatInKB("MemTotal")
})

// Get the amount of currently free memory in the host in KB. This is a lower
// bound for how much more memory we can theoretically use, since some cached
// files can be evicted to free up some more memory.
var GetFreeMemoryInKB = func() (string, error) {
	return calculateMemoryStatInKB("MemFree")
}

// Get the amount of currently available memory in the host in KB. This is a
// hard upper bound for how much more memory we can use before we start
// swapping or crash. We should stay well below this number because performance
// will suffer long before we get here.
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
var GetAwsAmiId = memoizeString(generateAWSMetadataRetriever("ami-id"))
var GetAwsAmiLaunchIndex = memoizeString(generateAWSMetadataRetriever("ami-launch-index"))
var GetAwsInstanceId = memoizeString(generateAWSMetadataRetriever("instance-id"))
var GetAwsInstanceType = memoizeString(generateAWSMetadataRetriever("instance-type"))
var GetAwsPlacementRegion = memoizeString(generateAWSMetadataRetriever("placement/region"))
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
		} else {
			if result, err := f(); err != nil {
				return result, err
			} else {
				cachedResult = result
				cached = true
				return cachedResult, nil
			}
		}
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
