package hosts

import (
	"context"
	"time"

	"github.com/aws/aws-sdk-go-v2/aws"
	"github.com/aws/aws-sdk-go-v2/config"
	"github.com/aws/aws-sdk-go-v2/service/ec2"
	ec2Types "github.com/aws/aws-sdk-go-v2/service/ec2/types"
	"github.com/whisthq/whist/backend/core-go/subscriptions"
	"github.com/whisthq/whist/backend/core-go/utils"
	logger "github.com/whisthq/whist/backend/core-go/whistlogger"
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

// SpinDownInstances is responsible for launching `numInstances` number of instances with the received imageID.
func (host *AWSHost) SpinUpInstances(numInstances *int32, imageID string) ([]subscriptions.Instance, error) {
	ctx := context.Background()

	// Set run input
	runInput := &ec2.RunInstancesInput{
		MaxCount:                          numInstances,
		ImageId:                           &imageID,
		InstanceInitiatedShutdownBehavior: ec2Types.ShutdownBehaviorTerminate,
	}

	runOutput, err := host.EC2.RunInstances(ctx, runInput)

	// Create slice with created instances
	var outputInstances []subscriptions.Instance

	for _, outputInstance := range runOutput.Instances {
		outputInstances = append(outputInstances, subscriptions.Instance{
			IP:              *outputInstance.PublicIpAddress,
			ImageID:         *outputInstance.ImageId,
			Type:            *outputInstance.PlatformDetails,
			CloudProviderID: *outputInstance.InstanceId,
			Name:            *outputInstance.Tags[0].Value,
			Status:          "PRE_CONNECTION",
		})
	}

	// Verify start output
	startedInstances := int(*numInstances)
	if len(runOutput.Instances) != startedInstances {
		return outputInstances,
			utils.MakeError("failed to start requested number of instances with parameters: %v. Number of started instances: %v",
				runInput, len(runOutput.Instances))
	}

	if err != nil {
		return outputInstances, utils.MakeError("error starting instances with parameters: %v. Error: %v", runInput, err)
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
			CloudProviderID: *outputInstance.InstanceId,
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

// WaitForInstanceTermination waits until the given instance has been terminated on the
// cloud service
func (host *AWSHost) WaitForInstanceTermination(scalingCtx context.Context, instance subscriptions.Instance) error {
	waiterClient := new(ec2.DescribeInstancesAPIClient)
	waiter := ec2.NewInstanceTerminatedWaiter(*waiterClient, func(*ec2.InstanceTerminatedWaiterOptions) {
		logger.Infof("Waiting for instance to terminate on AWS")
	})

	waitParams := &ec2.DescribeInstancesInput{
		InstanceIds: []string{instance.Name},
	}

	err := waiter.Wait(scalingCtx, waitParams, 5*time.Minute)
	if err != nil {
		return utils.MakeError("failed waiting for instance %v to terminate from AWS: %v", instance.Name, err)
	}

	return nil
}
