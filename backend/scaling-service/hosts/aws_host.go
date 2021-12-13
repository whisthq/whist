package hosts

import (
	"context"

	"github.com/aws/aws-sdk-go-v2/aws"
	"github.com/aws/aws-sdk-go-v2/config"
	"github.com/aws/aws-sdk-go-v2/service/ec2"
	"github.com/whisthq/whist/core-go/utils"
)

type AWSHost struct {
	Region string
	Config aws.Config
	EC2    *ec2.Client
}

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

func (host *AWSHost) SpinUpInstances(region string, numInstances int) (int, error) {
	ctx := context.Background()

	// Set start input
	startInput := &ec2.StartInstancesInput{}
	for i := 0; i < numInstances; i++ {

		startOutput, err := host.EC2.StartInstances(ctx, startInput)

		// Verify start output
		if startOutput == nil {

		}

		if err != nil {
			return 0, utils.MakeError("error starting instances with parameters: %v. Error: %v", startInput, err)
		}
	}
	return 0, nil
}

func (host *AWSHost) SpinDownInstance(instanceID string) error {
	ctx := context.Background()

	terminationParams := &ec2.TerminateInstancesInput{
		InstanceIds: []string{instanceID},
	}

	terminationOutput, err := host.EC2.TerminateInstances(ctx, terminationParams)

	// Verify termination output
	if terminationOutput == nil {

	}

	if err != nil {
		return utils.MakeError("error terminating instance with id: %v. Error: %v", instanceID, err)
	}
	return nil
}
