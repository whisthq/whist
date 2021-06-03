package fractallogger // import "github.com/fractal/fractal/ecs-host-service/fractallogger"

import (
	"github.com/aws/aws-sdk-go-v2/config"
	"github.com/aws/aws-sdk-go-v2/service/ec2"
	"github.com/aws/aws-sdk-go-v2/service/ec2/types"

	"context"
)

// TODO: rename this file, or otherwise organize this with the other AWS stuff

func GetInstanceName() (string, error) {
	cfg, err := config.LoadDefaultConfig(context.TODO(), config.WithRegion("us-east-1"))
	if err != nil {
		return "", MakeError("Unable to load aws config: %s", err)
	}
	client := ec2.NewFromConfig(cfg)

	instanceID, err := GetAwsInstanceID()
	if err != nil {
		return "", MakeError("Unable to get aws instance ID: %s", err)
	}

	var resourceTypeStr string = "resource-type"
	var resourceIdStr string = "resource-id"

	out, err := client.DescribeTags(context.TODO(), &ec2.DescribeTagsInput{
		Filters: []types.Filter{
			{
				Name:   &resourceTypeStr,
				Values: []string{"instance"},
			},
			{
				Name:   &resourceIdStr,
				Values: []string{instanceID},
			},
		},
	})

	if err != nil {
		return "", MakeError("Error describing tags: %s", err)
	}

	if len(out.Tags) == 0 {
	}

	for _, t := range out.Tags {
		if *(t.Key) == "Name" {
			return *(t.Value), nil
		}
	}

	return "", MakeError(`Did not find a "Name" tag for this instance!`)
}
