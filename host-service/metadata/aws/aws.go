package aws // import "github.com/fractal/fractal/host-service/metadata/aws"

import (
	"context"
	"io"
	"net"
	"net/http"
	"time"

	"github.com/aws/aws-sdk-go-v2/config"
	"github.com/aws/aws-sdk-go-v2/service/ec2"
	"github.com/aws/aws-sdk-go-v2/service/ec2/types"

	"github.com/fractal/fractal/host-service/utils"
)

// This file is laid out as a series of types corresponding to various
// AWS-specific metadata. Each type is accompanied by the functions needed to
// provide a "Get" method for the type. The bottom of the file contains any
// necessary helper functions.

// As soon as go has generics this file will look very different. ;)

//===============

// An AmiID represents the AWS AMI ID for this host.
type AmiID string

var getAmiIDStr = utils.MemoizeStringWithError(generateAWSMetadataRetriever("ami-id"))

// GetAmiID returns the AMI ID of the current EC2 instance.
func GetAmiID() (AmiID, error) {
	str, err := getAmiIDStr()
	return AmiID(str), err
}

//===============

// An InstanceID represents an AWS Instance ID.
type InstanceID string

var getInstanceIDStr = utils.MemoizeStringWithError(generateAWSMetadataRetriever("instance-id"))

// GetInstanceID returns the Instance ID of the. current EC2 instance
func GetInstanceID() (InstanceID, error) {
	str, err := getInstanceIDStr()
	return InstanceID(str), err
}

//===============

// An InstanceType represents an EC2 instance type (e.g. `g3s.xlarge`).
type InstanceType string

var getInstanceTypeStr = utils.MemoizeStringWithError(generateAWSMetadataRetriever("instance-type"))

// GetInstanceType returns the type of the current EC2 instance (e.g. g3s.xlarge).
func GetInstanceType() (InstanceType, error) {
	str, err := getInstanceTypeStr()
	return InstanceType(str), err
}

//===============

// An PlacementRegion represents the placement region of an EC2 instance (e.g. `us-east-1`).
type PlacementRegion string

var getPlacementRegionStr = utils.MemoizeStringWithError(generateAWSMetadataRetriever("placement/region"))

// GetPlacementRegion returns the placement region of the current EC2 instance (e.g. `us-east-1`).
func GetPlacementRegion() (PlacementRegion, error) {
	str, err := getPlacementRegionStr()
	return PlacementRegion(str), err
}

//===============

var getPublicIpv4 = utils.MemoizeStringWithError(generateAWSMetadataRetriever("public-ipv4"))

// GetPublicIpv4 returns the public IPv4 address of the current EC2 instance.
func GetPublicIpv4() (net.IP, error) {
	str, err := getPublicIpv4()
	if err != nil {
		return net.IP{}, err
	}
	ip := net.ParseIP(str).To4()
	if ip == nil {
		return ip, utils.MakeError("Unable to parse response from IMDS endpoint as an IPv4 address: %v", str)
	}
	return ip, nil
}

//===============

// An InstanceName represents the "Name" tag assigned to an instance on
// creation (i.e. what shows up in the EC2 console as the name of the
// instance).
type InstanceName string

var getInstanceName = utils.MemoizeStringWithError(func() (string, error) {
	region, err := GetPlacementRegion()
	if err != nil {
		return "", utils.MakeError("Unable to find region to create AWS SDK config!")
	}

	// Initialize general AWS config and ec2 client
	cfg, err := config.LoadDefaultConfig(context.Background(), config.WithRegion(string(region)))
	if err != nil {
		return "", utils.MakeError("Unable to load AWS SDK config: %s", err)
	}
	ec2Client := ec2.NewFromConfig(cfg)

	instanceID, err := GetInstanceID()
	if err != nil {
		return "", utils.MakeError("Unable to compute instance name because unable to get AWS instance ID: %s", err)
	}

	var resourceTypeStr string = "resource-type"
	var resourceIDStr string = "resource-id"

	out, err := ec2Client.DescribeTags(context.TODO(), &ec2.DescribeTagsInput{
		Filters: []types.Filter{
			{
				Name:   &resourceTypeStr,
				Values: []string{"instance"},
			},
			{
				Name:   &resourceIDStr,
				Values: []string{string(instanceID)},
			},
		},
	})

	if err != nil {
		return "", utils.MakeError("Error describing tags: %s", err)
	}

	for _, t := range out.Tags {
		if *(t.Key) == "Name" {
			return *(t.Value), nil
		}
	}

	// "Name" tag wasn't found, so we print out all the ones we did find.
	printableTags := make(map[string]string)
	for _, t := range out.Tags {
		printableTags[*(t.Key)] = *(t.Value)
	}

	return "", utils.MakeError(`Did not find a "Name" tag for this instance! Found these tags instead: %v`, printableTags)
})

// GetInstanceName returns the "Name" tag of the current EC2 instance.
func GetInstanceName() (InstanceName, error) {
	str, err := getInstanceName()
	return InstanceName(str), err
}

//===============

// This helper function generates functions that retrieve metadata from the
// internal AWS endpoint. We use this together with
// utils.MemoizeStringWithError() to expose memoized functions that query
// information about the host from AWS.  We use this setup so that we only have
// to query the AWS endpoint successfully once to get information for our
// startup, shutdown, and heartbeat messages. This architecture is compelling
// because of reports online that the AWS endpoint to query this information
// has transient failures. We want to minimize the impact of these transient
// failures on our application --- with this approach, host startup will be
// affected by these failures, but transient failures should not affect already
// running hosts.
func generateAWSMetadataRetriever(path string) func() (string, error) {
	// Note that we could use the ec2imds package of the AWS SDK, but that's
	// proven unnecessary over months of usage of the (simple) below code in
	// production so far. In the future, we could make the migration at the price
	// of a small increase in code complexity.

	const AWSendpointBase = "http://169.254.169.254/latest/meta-data/"
	httpClient := http.Client{
		Timeout: 1 * time.Second,
	}

	url := AWSendpointBase + path
	return func() (string, error) {
		resp, err := httpClient.Get(url)
		if err != nil {
			return "", utils.MakeError("Error retrieving data from URL %s: %v. Is the service running on an AWS EC2 instance?", url, err)
		}
		defer resp.Body.Close()

		body, err := io.ReadAll(resp.Body)
		if err != nil {
			return string(body), utils.MakeError("Error reading response body from URL %s: %v", url, err)
		}
		if resp.StatusCode != 200 {
			return string(body), utils.MakeError("Got non-200 response from URL %s: %s", url, resp.Status)
		}
		return string(body), nil
	}
}
