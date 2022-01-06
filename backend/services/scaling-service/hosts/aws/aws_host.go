package hosts

import (
	"context"
	"time"

	"github.com/aws/aws-sdk-go-v2/aws"
	"github.com/aws/aws-sdk-go-v2/config"
	"github.com/aws/aws-sdk-go-v2/service/ec2"
	ec2Types "github.com/aws/aws-sdk-go-v2/service/ec2/types"
	"github.com/lithammer/shortuuid"
	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/subscriptions"
	"github.com/whisthq/whist/backend/services/utils"
	logger "github.com/whisthq/whist/backend/services/whistlogger"
)

type AWSHost struct {
	Region string
	Config aws.Config
	EC2    *ec2.Client
}

// Initialize starts the AWS and EC2 clients.
func (host *AWSHost) Initialize(region string) error {
	// Initialize general AWS config on the selected region
	cfg, err := config.LoadDefaultConfig(context.Background(), config.WithRegion(region))
	if err != nil {
		return utils.MakeError("Unable to load AWS SDK config: %s", err)
	}

	// Start AWS host and EC2 client
	host.Region = region
	host.Config = cfg
	host.EC2 = ec2.NewFromConfig(cfg)

	return nil
}

func (host *AWSHost) MakeInstances(c context.Context, input *ec2.RunInstancesInput) (*ec2.RunInstancesOutput, error) {
	return host.EC2.RunInstances(c, input)
}

func (host *AWSHost) MakeTags(c context.Context, input *ec2.CreateTagsInput) (*ec2.CreateTagsOutput, error) {
	return host.EC2.CreateTags(c, input)
}

// SpinDownInstances is responsible for launching `numInstances` number of instances with the received imageID.
func (host *AWSHost) SpinUpInstances(scalingCtx context.Context, numInstances int32, imageID string) ([]subscriptions.Instance, error) {
	// Set run input
	input := &ec2.RunInstancesInput{
		MinCount:                          aws.Int32(MIN_INSTANCE_COUNT),
		MaxCount:                          aws.Int32(numInstances),
		ImageId:                           aws.String(imageID),
		InstanceInitiatedShutdownBehavior: ec2Types.ShutdownBehaviorTerminate,
		InstanceType:                      INSTANCE_TYPE,
	}

	var (
		attempts int
		result   *ec2.RunInstancesOutput
		err      error
	)
	retryTicker := time.NewTicker(WAIT_TIME_BEFORE_RETRY_IN_SECONDS * time.Second)
	retryDone := make(chan bool)

	go func() {
		for {
			select {
			case <-retryDone:
				logger.Infof("Done retrying for instance spinup.")
				retryTicker.Stop()

			case <-retryTicker.C:
				logger.Infof("Trying to spinup %v instances with image %v", input.MaxCount, input.ImageId)

				attempts += 1
				result, err = host.MakeInstances(scalingCtx, input)

				if err == nil || attempts == MAX_RETRY_ATTEMPTS {
					retryDone <- true
				} else {
					logger.Warningf("Failed to start desired number of instances with error: %v", err)
				}
			}
		}
	}()

	<-retryDone

	if err != nil {
		return nil, utils.MakeError("error creating instances, retry time expired: Err: %v", err)
	}

	// Create slice with instances to write to database
	var outputInstances []subscriptions.Instance

	for _, outputInstance := range result.Instances {
		var (
			IP      = aws.ToString(outputInstance.PublicIpAddress)
			ImageID = aws.ToString(outputInstance.ImageId)
			// Type       = aws.ToString(outputInstance.PlatformDetails)
			InstanceID = aws.ToString(outputInstance.InstanceId)
			Status     = "PRE_CONNECTION"
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

		_, err = host.MakeTags(scalingCtx, tagInput)
		if err != nil {
			return nil, utils.MakeError("error taging instance. Err: %v", err)
		}

		// Only append instance to output slice if we have created
		// and tagged su
		outputInstances = append(outputInstances, subscriptions.Instance{
			IPAddress: IP,
			ImageID:   ImageID,
			// Type:            Type,
			ID:     InstanceID,
			Status: Status,
			// Name:            Name,
		})
	}

	// Verify start output
	desiredInstances := int(numInstances)
	if len(outputInstances) != desiredInstances {
		return outputInstances,
			utils.MakeError("failed to start requested number of instances with parameters: %v. Number of instances marked as ready: %v",
				input, len(result.Instances))
	}

	if err != nil {
		return outputInstances, utils.MakeError("error starting instances with parameters: %v. Error: %v", input, err)
	}
	return outputInstances, nil
}

// SpinDownInstances is responsible for terminating the instances in `instanceIDs`.
func (host *AWSHost) SpinDownInstances(scalingCtx context.Context, instanceIDs []string) ([]subscriptions.Instance, error) {

	terminateInput := &ec2.TerminateInstancesInput{
		InstanceIds: instanceIDs,
	}

	terminateOutput, err := host.EC2.TerminateInstances(scalingCtx, terminateInput)

	// Create slice with created instances
	var outputInstances []subscriptions.Instance

	for _, outputInstance := range terminateOutput.TerminatingInstances {
		outputInstances = append(outputInstances, subscriptions.Instance{
			ID: *outputInstance.InstanceId,
		})
	}

	// Verify termination output
	if len(terminateOutput.TerminatingInstances) != len(instanceIDs) {
		return outputInstances, utils.MakeError("failed to terminate requested number of instances with parameters: %v. Number of terminated instances: %v",
			terminateInput, len(terminateOutput.TerminatingInstances))
	}

	if err != nil {
		return outputInstances, utils.MakeError("error terminating instance with id: %v. Error: %v", instanceIDs, err)
	}
	return outputInstances, nil
}

// WaitForInstanceTermination waits until the given instance has been terminated on AWS.
func (host *AWSHost) WaitForInstanceTermination(scalingCtx context.Context, instanceIds []string) error {
	waiter := ec2.NewInstanceTerminatedWaiter(host.EC2, func(*ec2.InstanceTerminatedWaiterOptions) {
		logger.Infof("Waiting for instances to terminate on AWS")
	})

	waitParams := &ec2.DescribeInstancesInput{
		InstanceIds: instanceIds,
	}

	err := waiter.Wait(scalingCtx, waitParams, 5*time.Minute)
	if err != nil {
		return utils.MakeError("failed waiting for instance %v to terminate from AWS: %v", instanceIds, err)
	}

	return nil
}

// WaitForInstanceReady waits until the given instance is running on AWS.
func (host *AWSHost) WaitForInstanceReady(scalingCtx context.Context, instanceIds []string) error {
	waiter := ec2.NewInstanceRunningWaiter(host.EC2, func(*ec2.InstanceRunningWaiterOptions) {
		logger.Infof("Waiting for instances %v to be ready on AWS", instanceIds)
	})

	waitParams := &ec2.DescribeInstancesInput{
		InstanceIds: instanceIds,
	}

	err := waiter.Wait(scalingCtx, waitParams, 5*time.Minute)
	if err != nil {
		return utils.MakeError("failed waiting for instance %v to be ready from AWS: %v", instanceIds, err)
	}

	return nil
}

// GenerateName is a helper function for generating an instance
// name using a random UUID.
func (host *AWSHost) GenerateName() string {
	return utils.Sprintf("ec2-%v-%v-%v%v", host.Region, metadata.GetAppEnvironmentLowercase(),
		metadata.GetGitCommit(), shortuuid.New())
}
