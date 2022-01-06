package hosts

import (
	"context"
	"time"

	"github.com/aws/aws-sdk-go-v2/aws"
	"github.com/aws/aws-sdk-go-v2/config"
	"github.com/aws/aws-sdk-go-v2/service/ec2"
	ec2Types "github.com/aws/aws-sdk-go-v2/service/ec2/types"
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

func MakeInstance(c context.Context, host *AWSHost, input *ec2.RunInstancesInput) (*ec2.RunInstancesOutput, error) {
	return host.EC2.RunInstances(c, input)
}

func MakeTags(c context.Context, host *AWSHost, input *ec2.CreateTagsInput) (*ec2.CreateTagsOutput, error) {
	return host.EC2.CreateTags(c, input)
}

// SpinDownInstances is responsible for launching `numInstances` number of instances with the received imageID.
func (host *AWSHost) SpinUpInstances(numInstances int32, imageID string) ([]subscriptions.Host, error) {
	ctx := context.Background()

	// Set run input
	input := &ec2.RunInstancesInput{
		MinCount:                          aws.Int32(MIN_INSTANCE_COUNT),
		MaxCount:                          aws.Int32(numInstances),
		ImageId:                           aws.String(imageID),
		InstanceInitiatedShutdownBehavior: ec2Types.ShutdownBehaviorTerminate,
		InstanceType:                      INSTANCE_TYPE,
	}

	result, err := MakeInstance(ctx, host, input)
	if err != nil {
		return nil, utils.MakeError("error creating instances: Err: %v", err)
	}

	tagInput := &ec2.CreateTagsInput{
		Resources: []string{*result.Instances[0].InstanceId},
		Tags: []ec2Types.Tag{
			{
				Key:   aws.String("name"),
				Value: aws.String("scaling-service-instance"),
			},
		},
	}

	_, err = MakeTags(ctx, host, tagInput)
	if err != nil {
		return nil, utils.MakeError("error taging instance. Err: %v", err)
	}

	logger.Infof("Created tagged instance with ID " + *result.Instances[0].InstanceId)

	// Create slice with created instances
	var outputInstances []subscriptions.Host

	for _, outputInstance := range result.Instances {
		outputInstances = append(outputInstances, subscriptions.Instance{
			IPAddress: *outputInstance.PublicIpAddress,
			ImageID:   *outputInstance.ImageId,
			// Type:            *outputInstance.PlatformDetails,
			ID: *outputInstance.InstanceId,
			// Name:   *outputInstance.Tags[0].Value,
			Status: "PRE_CONNECTION",
		})
	}

	// Verify start output
	startedInstances := int(numInstances)
	if len(result.Instances) != startedInstances {
		return outputInstances,
			utils.MakeError("failed to start requested number of instances with parameters: %v. Number of started instances: %v",
				input, len(result.Instances))
	}

	if err != nil {
		return outputInstances, utils.MakeError("error starting instances with parameters: %v. Error: %v", input, err)
	}
	return outputInstances, nil
}

// SpinDownInstances is responsible for terminating the instances in `instanceIDs`.
func (host *AWSHost) SpinDownInstances(instanceIDs []string) ([]subscriptions.Instance, error) {
	ctx := context.Background()

	terminateInput := &ec2.TerminateInstancesInput{
		InstanceIds: instanceIDs,
	}

	terminateOutput, err := host.EC2.TerminateInstances(ctx, terminateInput)

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
func (host *AWSHost) WaitForInstanceTermination(scalingCtx context.Context, instance subscriptions.Instance) error {
	waiterClient := new(ec2.DescribeInstancesAPIClient)
	waiter := ec2.NewInstanceTerminatedWaiter(*waiterClient, func(*ec2.InstanceTerminatedWaiterOptions) {
		logger.Infof("Waiting for instance to terminate on AWS")
	})

	waitParams := &ec2.DescribeInstancesInput{
		InstanceIds: []string{instance.ID},
	}

	err := waiter.Wait(scalingCtx, waitParams, 5*time.Minute)
	if err != nil {
		return utils.MakeError("failed waiting for instance %v to terminate from AWS: %v", instance.ID, err)
	}

	return nil
}

// WaitForInstanceReady waits until the given instance is running on AWS.
func (host *AWSHost) WaitForInstanceReady(scalingCtx context.Context, instance subscriptions.Instance) error {
	waiterClient := new(ec2.DescribeInstancesAPIClient)
	waiter := ec2.NewInstanceRunningWaiter(*waiterClient, func(*ec2.InstanceRunningWaiterOptions) {
		logger.Infof("Waiting for instance to be ready on AWS")
	})

	waitParams := &ec2.DescribeInstancesInput{
		InstanceIds: []string{instance.ID},
	}

	err := waiter.Wait(scalingCtx, waitParams, 5*time.Minute)
	if err != nil {
		return utils.MakeError("failed waiting for instance %v to be ready from AWS: %v", instance.ID, err)
	}

	return nil
}

func (host *AWSHost) RetryInstanceSpinUp() {

}
