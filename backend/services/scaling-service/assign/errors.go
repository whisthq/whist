package assign

const (
	// The override value used when assigning users in dev, for developer convenience.
	CLIENT_COMMIT_HASH_DEV_OVERRIDE = "local_dev"

	// These are all the possible reasons we would fail to find an instance for a user
	// and return a 503 error

	// Instance was found but the frontend is out of date
	COMMIT_HASH_MISMATCH = "COMMIT_HASH_MISMATCH"

	// No instance was found e.g. a capacity issue
	NO_INSTANCE_AVAILABLE = "NO_INSTANCE_AVAILABLE"

	// The requested region(s) have not been enabled
	REGION_NOT_ENABLED = "REGION_NOT_ENABLED"

	// User is already connected to a mandelbox, possibly on another device
	USER_ALREADY_ACTIVE = "USER_ALREADY_ACTIVE"

	// A generic 503 error message
	SERVICE_UNAVAILABLE = "SERVICE_UNAVAILABLE"
)
