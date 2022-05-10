package aws

import ec2Types "github.com/aws/aws-sdk-go-v2/service/ec2/types"

// Configuration for retry logic

const (
	WAIT_TIME_BEFORE_RETRY_IN_SECONDS = 15
	MAX_WAIT_TIME_IN_SECONDS          = 200
	MAX_RETRY_ATTEMPTS                = MAX_WAIT_TIME_IN_SECONDS / WAIT_TIME_BEFORE_RETRY_IN_SECONDS
)

// Configuration for instances

const (
	// The minimum of instances to launch. Necessary for the AWS SDK.
	MIN_INSTANCE_COUNT = 1

	// The type of instances we want to launch. TODO: change data structure when
	// different instance types are added.
	INSTANCE_TYPE = ec2Types.InstanceTypeG4dn2xlarge
)
