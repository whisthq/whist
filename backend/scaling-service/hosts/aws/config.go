package hosts

// We want to have multiple attempts at starting an instance
// if it fails due to insufficient capacity.
const (
	WAIT_TIME_BEFORE_RETRY_IN_SECONDS = 15
	MAX_WAIT_TIME_IN_SECONDS          = 200
	MAX_RETRY_ATTEMPTS                = MAX_WAIT_TIME_IN_SECONDS / WAIT_TIME_BEFORE_RETRY_IN_SECONDS
)
