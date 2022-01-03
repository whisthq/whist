package hosts

import ec2Types "github.com/aws/aws-sdk-go-v2/service/ec2/types"

// We want to have multiple attempts at starting an instance
// if it fails due to insufficient capacity.
const (
	WAIT_TIME_BEFORE_RETRY_IN_SECONDS = 15
	MAX_WAIT_TIME_IN_SECONDS          = 200
	MAX_RETRY_ATTEMPTS                = MAX_WAIT_TIME_IN_SECONDS / WAIT_TIME_BEFORE_RETRY_IN_SECONDS
)

// Configuration for instances
const (
	MIN_INSTANCE_COUNT = 1
	INSTANCE_TYPE      = ec2Types.InstanceTypeG4dn2xlarge
)
