/*
This file implements the HostHandler interface and defines all necessary methods
that launch, wait and manage instances using the EC2 service.
The code uses the official AWS SDK for Go (https://github.com/aws/aws-sdk-go-v2).
*/

package hosts

import (
	"context"
	_ "embed"
	"encoding/base64"
	"os"
	"time"

	"github.com/aws/aws-sdk-go-v2/aws"
	"github.com/aws/aws-sdk-go-v2/config"
	"github.com/aws/aws-sdk-go-v2/service/ec2"
	ec2Types "github.com/aws/aws-sdk-go-v2/service/ec2/types"
	shortuuid "github.com/lithammer/shortuuid/v3"
	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/subscriptions"
	"github.com/whisthq/whist/backend/services/utils"
	logger "github.com/whisthq/whist/backend/services/whistlogger"
)

// AWSHost implements the HostHandler interface using the AWS sdk.
type AWSHost struct {
	// The AWS region the host is initialized in.
	region string
	// The AWS specific configurations, including credentials.
	Config aws.Config
	// The EC2 client used for calling the AWS EC2 service API.
	EC2 *ec2.Client
	// The main subnet where the scaling algorithm starts instances.
	MainSubnet ec2Types.Subnet
	// The AWS Instance Profile passed to each instance.
	InstanceProfile string
	// The Security Group that manages instance ingress and egress rules.
	SecurityGroup ec2Types.SecurityGroup
}

//go:embed ec2_userdata.sh
var userDataFile []byte

// Initialize starts the AWS and EC2 clients.
func (host *AWSHost) Initialize(region string) error {
	// Initialize general AWS config on the selected region
	cfg, err := config.LoadDefaultConfig(context.Background(), config.WithRegion(region))
	if err != nil {
		return utils.MakeError("unable to load AWS SDK config: %s", err)
	}

	// Start AWS host and EC2 client
	host.region = region
	host.Config = cfg
	host.EC2 = ec2.NewFromConfig(cfg)

	// Get main subnet for the current region and env
	mainSubnet, err := host.getMainSubnet()
	if err != nil {
		return err
	}

	// Get security group for the current region and env
	securityGroup, err := host.getSecurityGroup()
	if err != nil {
		return err
	}

	// Get instance profile for current env
	profile := host.getInstanceProfile()
	if profile == "" {
		return utils.MakeError("instance profile is empty")
	}

	host.MainSubnet = mainSubnet
	host.SecurityGroup = securityGroup
	host.InstanceProfile = profile

	return nil
}

// GetRegion is part of the HostHandler interface.
func (h *AWSHost) GetRegion() string {
	return h.region
}

// MakeInstances is a simple method that calls the RunInstances function from the ec2 client.
func (host *AWSHost) MakeInstances(c context.Context, input *ec2.RunInstancesInput) (*ec2.RunInstancesOutput, error) {
	return host.EC2.RunInstances(c, input)
}

// MakeInstances is a simple method that calls the CreateTags function from the ec2 client.
func (host *AWSHost) MakeTags(c context.Context, input *ec2.CreateTagsInput) (*ec2.CreateTagsOutput, error) {
	return host.EC2.CreateTags(c, input)
}

// SpinDownInstances is responsible for launching `numInstances` number of instances with the received imageID.
func (host *AWSHost) SpinUpInstances(scalingCtx context.Context, numInstances int32, maxWaitTimeReady time.Duration, image subscriptions.Image) ([]subscriptions.Instance, error) {
	ctx, cancel := context.WithCancel(scalingCtx)
	defer cancel()

	// Get base64 encoded user data to start host service at launch
	userData, err := getUserData()
	if err != nil {
		return nil, utils.MakeError("failed to encode userdata for launching instances: %s", err)
	}

	// Set run input
	input := &ec2.RunInstancesInput{
		MinCount:                          aws.Int32(MIN_INSTANCE_COUNT),
		MaxCount:                          aws.Int32(numInstances),
		ImageId:                           aws.String(image.ImageID),
		InstanceInitiatedShutdownBehavior: ec2Types.ShutdownBehaviorTerminate,
		InstanceType:                      INSTANCE_TYPE,
		IamInstanceProfile: &ec2Types.IamInstanceProfileSpecification{
			Arn: aws.String(host.InstanceProfile),
		},
		SubnetId: host.MainSubnet.SubnetId,
		SecurityGroupIds: []string{
			aws.ToString(host.SecurityGroup.GroupId),
		},
		UserData:     aws.String(userData),
		EbsOptimized: aws.Bool(true), // This has to be set at launch time, otherwise AWS will disable the optimization
	}

	var result *ec2.RunInstancesOutput

	for i := 0; i < MAX_RETRY_ATTEMPTS; i++ {
		logger.Infof("Attempt %d to spinup %d instances with image %s", i+1, numInstances, image.ImageID)
		result, err = host.MakeInstances(ctx, input)

		if err == nil {
			logger.Infof("Done trying to spinup instances")
			break
		} else {
			logger.Warningf("Failed to start instances: %s", err)
			time.Sleep(1 * time.Second)
		}
	}

	// If the last value of err is not nil, we know the loop failed to
	// spinup instances.
	if err != nil {
		return nil, err
	}

	// Create slice with instances to write to database
	var (
		outputInstances   []subscriptions.Instance
		outputInstanceIDs []string
	)

	// Get IDs of all created instances
	for _, outputInstance := range result.Instances {
		outputInstanceIDs = append(outputInstanceIDs, aws.ToString(outputInstance.InstanceId))
	}

	err = host.WaitForInstanceReady(scalingCtx, maxWaitTimeReady, outputInstanceIDs)
	if err != nil {
		return nil, utils.MakeError("failed to wait for new instances to be ready: %s", err)
	}

	for _, outputInstance := range result.Instances {
		var (
			ImageID    = aws.ToString(outputInstance.ImageId)
			Type       = string(outputInstance.InstanceType)
			InstanceID = aws.ToString(outputInstance.InstanceId)
			Region     = host.region
			Status     = "PRE_CONNECTION"
			ClientSHA  = image.ClientSHA
			Name       = host.GenerateName()
		)

		tagInput := &ec2.CreateTagsInput{
			Resources: []string{InstanceID},
			Tags: []ec2Types.Tag{
				{
					Key:   aws.String("Name"),
					Value: aws.String(Name),
				},
			},
		}

		_, err = host.MakeTags(ctx, tagInput)
		if err != nil {
			return nil, utils.MakeError("error taging instance: %s", err)
		}

		// Only append instance to output slice if we have created
		// and tagged successfully.
		outputInstances = append(outputInstances, subscriptions.Instance{
			ID:        InstanceID,
			IPAddress: "0.0.0.0", // Use dummy, will be set by host-service.
			Region:    Region,
			Provider:  "AWS",
			ImageID:   ImageID,
			ClientSHA: ClientSHA,
			Type:      Type,
			Status:    Status,
		})
	}

	// Verify start output
	desiredInstances := int(numInstances)
	if len(outputInstances) != desiredInstances {
		return outputInstances,
			utils.MakeError("failed to start requested number of instances, only marked as ready %d", len(result.Instances))
	}

	if err != nil {
		return outputInstances, utils.MakeError("error starting instances: %s", err)
	}
	return outputInstances, nil
}

// SpinDownInstances is responsible for terminating the instances in `instanceIDs`.
func (host *AWSHost) SpinDownInstances(scalingCtx context.Context, instanceIDs []string) error {
	ctx, cancel := context.WithCancel(scalingCtx)
	defer cancel()

	terminateInput := &ec2.TerminateInstancesInput{
		InstanceIds: instanceIDs,
	}

	terminateOutput, err := host.EC2.TerminateInstances(ctx, terminateInput)
	if err != nil {
		return utils.MakeError("error terminating instance %s: %s", instanceIDs, err)
	}

	// Verify termination output
	if len(terminateOutput.TerminatingInstances) != len(instanceIDs) {
		return utils.MakeError("failed to terminate requested number of instances, only terminated %d", len(terminateOutput.TerminatingInstances))
	}

	return nil
}

// WaitForInstanceTermination waits until the given instance has been terminated on AWS.
func (host *AWSHost) WaitForInstanceTermination(scalingCtx context.Context, maxWaitTime time.Duration, instanceIds []string) error {
	if metadata.IsLocalEnv() {
		return nil
	}

	ctx, cancel := context.WithCancel(scalingCtx)
	defer cancel()

	waiter := ec2.NewInstanceTerminatedWaiter(host.EC2, func(*ec2.InstanceTerminatedWaiterOptions) {
		logger.Infof("Waiting for instances to terminate")
	})

	waitParams := &ec2.DescribeInstancesInput{
		InstanceIds: instanceIds,
	}

	err := waiter.Wait(ctx, waitParams, maxWaitTime)
	if err != nil {
		return utils.MakeError("failed waiting for instances to terminate: %s", err)
	}

	return nil
}

// WaitForInstanceReady waits until the given instance is running on AWS.
func (host *AWSHost) WaitForInstanceReady(scalingCtx context.Context, maxWaitTime time.Duration, instanceIds []string) error {
	ctx, cancel := context.WithCancel(scalingCtx)
	defer cancel()

	waiter := ec2.NewInstanceRunningWaiter(host.EC2, func(*ec2.InstanceRunningWaiterOptions) {
		logger.Infof("Waiting for instances to be ready", instanceIds)
	})

	waitParams := &ec2.DescribeInstancesInput{
		InstanceIds: instanceIds,
	}

	err := waiter.Wait(ctx, waitParams, maxWaitTime)
	if err != nil {
		return utils.MakeError("failed waiting for instances to be ready: %s", err)
	}

	return nil
}

// getMainSubnet returns the main subnet of the region for the current environment.
func (host *AWSHost) getMainSubnet() (ec2Types.Subnet, error) {
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()

	var env string

	if metadata.IsLocalEnv() {
		env = string(metadata.EnvDev)
	} else {
		env = metadata.GetAppEnvironmentLowercase()
	}

	// Find the main subnet for the current environment using tags.
	input := &ec2.DescribeSubnetsInput{
		Filters: []ec2Types.Filter{
			{
				Name:   aws.String("tag:Env"),
				Values: []string{env},
			},
			{
				Name:   aws.String("tag:Terraform"),
				Values: []string{"true"},
			},
		},
	}
	output, err := host.EC2.DescribeSubnets(ctx, input)
	if err != nil {
		return ec2Types.Subnet{}, utils.MakeError("failed to get main subnet for env %s: %s", env, err)
	}

	if len(output.Subnets) == 0 {
		return ec2Types.Subnet{}, utils.MakeError("couldn't find the main subnet for env %s", env)
	}
	return output.Subnets[0], nil
}

// getSecurityGroup returns the security group of the Whist VPC.
func (host *AWSHost) getSecurityGroup() (ec2Types.SecurityGroup, error) {
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()

	var env string

	if metadata.IsLocalEnv() {
		env = string(metadata.EnvDev)
	} else {
		env = metadata.GetAppEnvironmentLowercase()
	}

	// find the main subnet for the current environment using tags.
	input := &ec2.DescribeSecurityGroupsInput{
		Filters: []ec2Types.Filter{
			{
				Name: aws.String("tag:Name"),
				Values: []string{
					utils.Sprintf("MandelboxesSecurityGroup%s", env),
				},
			},
		},
	}
	output, err := host.EC2.DescribeSecurityGroups(ctx, input)
	if err != nil {
		return ec2Types.SecurityGroup{}, utils.MakeError("failed to get security group for env %s: %s", env, err)
	}

	if len(output.SecurityGroups) == 0 {
		return ec2Types.SecurityGroup{}, utils.MakeError("couldn't find security group for env %s", env)
	}
	return output.SecurityGroups[0], nil
}

// getInstanceProfile returns the arn of the instance profile to use.
func (host *AWSHost) getInstanceProfile() string {
	if metadata.IsLocalEnv() {
		logger.Infof("Running on localdev, using dummy instance profile...")
		return "dummy_instance_profile"
	}

	switch metadata.GetAppEnvironmentLowercase() {
	case string(metadata.EnvDev):
		return os.Getenv("INSTANCE_PROFILE_DEV")
	case string(metadata.EnvStaging):
		return os.Getenv("INSTANCE_PROFILE_STAGING")
	case string(metadata.EnvProd):
		return os.Getenv("INSTANCE_PROFILE_PROD")
	default:
		return os.Getenv("INSTANCE_PROFILE_DEV")
	}
}

// GenerateName is a helper function for generating an instance
// name using a random UUID.
func (host *AWSHost) GenerateName() string {
	return utils.Sprintf("ec2-%v-%v-%v-%v", host.region, metadata.GetAppEnvironmentLowercase(),
		metadata.GetGitCommit()[0:7], shortuuid.New())
}

// getUserData returns the base64 encoded userdata file to pass to instances.
func getUserData() (string, error) {
	// Return as a base64 encoded string to pass to the EC2 client
	return base64.StdEncoding.EncodeToString(userDataFile), nil
}
