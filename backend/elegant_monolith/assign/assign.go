package assign

import (
	context "context"
	"fmt"
	"net"
	"regexp"
	"time"

	"github.com/whisthq/whist/backend/elegant_monolith/internal"
	"github.com/whisthq/whist/backend/elegant_monolith/internal/config"
	"github.com/whisthq/whist/backend/elegant_monolith/pkg/logger"
	"golang.org/x/exp/constraints"
)

func (svc *assignService) MandelboxAssign(ctx context.Context, regions []string, userEmail string, userID string, commitHash string) (string, string, AssignError) {
	logger.Infof("Starting mandelbox assign action. %v %v %v %v", regions, userEmail, userID, commitHash)
	defer logger.Infof("Finished mandelbox assign action.")

	// Note: we receive the email from the client, so its value should
	// not be trusted for anything else other than logging since
	// it can be spoofed. We sanitize the email before using to help mitigate
	// potential attacks.
	unsafeEmail, err := sanitizeEmail(userEmail)
	if err != nil {
		return "", "", AssignError{ErrorCode: SERVICE_UNAVAILABLE, Err: err}
	}
	logger.Infof("Frontend reported email %s, this value might not be accurate and is untrusted.", unsafeEmail)

	shouldAllocateMandelbox, err := svc.checkForExistingMandelbox(ctx, userID)
	if err != nil {
		return "", "", AssignError{
			ErrorCode: SERVICE_UNAVAILABLE,
			Err:       fmt.Errorf("failed to get mandelboxes from database: %s", err),
		}
	}

	// Before proceeding with the assign process, find out if the user already has other mandelboxes
	// allocated.
	if !shouldAllocateMandelbox {
		err := fmt.Errorf("user %s already has mandelboxes allocated or running, so not assigning more mandelboxes", unsafeEmail)
		return "", "", AssignError{
			ErrorCode: USER_ALREADY_ACTIVE,
			Err:       err,
		}
	}

	var (
		requestedRegions   = regions         // This is a list of the regions requested by the frontend, in order of proximity.
		availableRegions   []string          // List of regions that are enabled and match the regions in the request.
		unavailableRegions []string          // List of regions that are not enabled but are present in the request.
		assignedInstance   internal.Instance // The instance that is assigned to the user.
	)

	// The number of elements  to truncate a slice of regions to. Used when logging unavailable region errors.
	const truncateTo = 3

	// Populate availableRegions
	for _, requestedRegion := range requestedRegions {
		var regionFound bool
		for _, enabledRegion := range config.GetEnabledRegions() {
			if enabledRegion == requestedRegion {
				availableRegions = append(availableRegions, requestedRegion)
				regionFound = true
			}
		}

		if !regionFound {
			unavailableRegions = append(unavailableRegions, requestedRegion)
		}
	}

	// This means that the user has requested access to some regions that are not yet enabled,
	// but could still be allocated to a region that is relatively close.
	if len(unavailableRegions) != 0 && len(unavailableRegions) != len(requestedRegions) {
		logger.Warningf("User %s requested access to the following unavailable regions: %s", unsafeEmail, printSlice(unavailableRegions, truncateTo))
	}

	// The user requested access to only unavailable regions. This means the user is far from
	// any of the available regions, and the frontend should handle that accordingly.
	if len(unavailableRegions) == len(requestedRegions) {
		err := fmt.Errorf("user %s requested access to only unavailable regions: %s", unsafeEmail, printSlice(unavailableRegions, truncateTo))
		return "", "", AssignError{
			ErrorCode: REGION_NOT_ENABLED,
			Err:       err,
		}
	}

	// TODO use latest image in localdev

	var (
		instanceFound          bool
		instanceProximityIndex int
	)

	// This is the "main" loop that does all the work and tries to find an instance for a user. first, it will iterate
	// over the list of regions provided on the request, and will query the database on each to return the list of
	// instances with capacity on the current region. Once it gets the instances, it will iterate over them and try
	// to find an instance with a matching commit hash. If it fails to do so, move on to the next region.
	for i, region := range availableRegions {
		// TODO: set different cloud provider when doing multi-cloud
		regionImage, rows, err := svc.db.QueryLatestImage(ctx, "AWS", region)
		if err != nil || rows == 0 {
			return "", "", AssignError{
				ErrorCode: SERVICE_UNAVAILABLE,
				Err:       fmt.Errorf("failed to query latest image in %s: %s", region, err),
			}
		}

		logger.Infof("Trying to find instance in region %s, with commit hash %s and image %s",
			region, commitHash, regionImage.ID)

		instanceResult, rows, err := svc.db.QueryInstancesWithCapacity(ctx, region)
		if err != nil {
			return "", "", AssignError{
				ErrorCode: SERVICE_UNAVAILABLE,
				Err:       fmt.Errorf("failed to query instances in %s: %s", region, err),
			}
		}

		if rows == 0 {
			logger.Warningf("Failed to find any instances with capacity in %s. Trying on next region", region)
			continue
		}

		// Iterate over available instances, try to find one with a matching commit hash and image.
		for i := range instanceResult {
			assignedInstance = instanceResult[i]

			if assignedInstance.ClientSHA == commitHash &&
				assignedInstance.ImageID == regionImage.ID {
				logger.Infof("Found instance %s with commit hash %s", assignedInstance.ID, assignedInstance.ClientSHA)
				instanceFound = true
				break
			}
			logger.Warningf("Found an instance in %s but it has a different commit hash %s. Trying on next region", region, assignedInstance.ClientSHA)
		}

		// Break of outer loop if instance was found. If no instance with
		// matching commit hash was found, move on to the next region.
		if instanceFound {
			// Persist the assigned region context
			instanceProximityIndex = i
			break
		}

		logger.Infof("No instances found in %s with commit hash %s", region, commitHash)
	}

	// No instances with capacity were found
	if assignedInstance == (internal.Instance{}) {
		err := fmt.Errorf("did not find an instance with capacity for user %s and commit hash %s", userEmail, commitHash)
		return "", "", AssignError{
			ErrorCode: NO_INSTANCE_AVAILABLE,
			Err:       err,
		}
	}

	// If the index of the region we assigned is not the first, it means the
	// user was not assigned to the closest available region (since the slice
	// is already sorted by proximity).
	if instanceProximityIndex > 0 {
		logger.Errorf("failed to assign user to closest region: assigned to %s instead of %s (region %d of %d)",
			assignedInstance.Region, availableRegions[0], instanceProximityIndex+1, len(availableRegions))
	} else {
		logger.Infof("Successfully assigned user to closest requested region %s", assignedInstance.Region)
	}

	// There are instances with capacity available, but none of them with the desired commit hash.
	// We only consider this error in cases when the frontend has a version greater or equal than
	// the one in the config database.
	if assignedInstance.ClientSHA != commitHash {
		return "", "", AssignError{
			ErrorCode: COMMIT_HASH_MISMATCH,
			Err:       fmt.Errorf("found instance with capacity but different commit hash"),
		}
	}

	waitingMandelbox, rows, err := svc.db.QueryMandelbox(ctx, assignedInstance.ID, internal.MandelboxStatusWaiting)
	if err != nil {
		logger.Errorf("failed to query database for mandelbox on instance %s: %s", assignedInstance.ID, err)
		return "", "", AssignError{
			ErrorCode: SERVICE_UNAVAILABLE,
			Err:       err,
		}
	}

	if rows == 0 {
		logger.Errorf("failed to find a waiting mandelbox even though the instance %s had sufficient capacity", assignedInstance.ID)
		return "", "", AssignError{
			ErrorCode: SERVICE_UNAVAILABLE,
			Err:       err,
		}
	}

	waitingMandelbox.UserID = userID
	waitingMandelbox.Status = internal.MandelboxStatusAllocated
	waitingMandelbox.UpdatedAt = time.Now()

	updated, err := svc.db.UpdateMandelbox(ctx, waitingMandelbox)
	if err != nil {
		logger.Errorf("error updating mandelbox %s: %s", waitingMandelbox.ID, err)
		return "", "", AssignError{
			ErrorCode: SERVICE_UNAVAILABLE,
			Err:       err,
		}
	}
	logger.Infof("Updated %d mandelbox rows in database", updated)

	assignedInstance.RemainingCapacity--
	updated, err = svc.db.UpdateInstance(ctx, assignedInstance)
	if err != nil {
		logger.Errorf("error updating instance %s: %s", assignedInstance.ID, err)
		return "", "", AssignError{
			ErrorCode: SERVICE_UNAVAILABLE,
			Err:       err,
		}
	}
	logger.Infof("Updated %d instance rows in database", updated)

	// Parse IP address. The database uses the CIDR notation (192.0.2.0/24)
	// so we need to extract the address and send it to the frontend.
	ip, _, err := net.ParseCIDR(assignedInstance.IPAddress)
	if err != nil {
		return "", "", AssignError{
			ErrorCode: SERVICE_UNAVAILABLE,
			Err:       fmt.Errorf("failed to parse IP address %s: %s", assignedInstance.IPAddress, err),
		}
	}

	return waitingMandelbox.ID, ip.String(), AssignError{}
}

// checkForExistingMandelbox queries the database to compute the current number of active mandelboxes associated to a single user.
// This function return a boolean indicating whether the user should be allocated a new CheckForExistingMandelbox or the request
// should be rejected if the mandelbox limit is exceeded. The caller is responsible for handling the response to the assign request.
func (svc *assignService) checkForExistingMandelbox(ctx context.Context, userID string) (bool, error) {
	if userID == "" {
		return false, fmt.Errorf("user ID is empty, not assigning mandelboxes")
	}

	mandelboxResult, rows, err := svc.db.QueryUserMandelboxes(ctx, userID)
	if err != nil {
		return false, err
	}

	logger.Infof("User %s has %d mandelboxes active", userID, len(mandelboxResult))

	if rows == 0 {
		return true, nil
	}

	var activeOrConnectingMandelboxes int

	for _, mandelbox := range mandelboxResult {
		// We consider all mandelboxes that are (or will be) running.
		if !(mandelbox.Status == internal.MandelboxStatusDying) {
			activeOrConnectingMandelboxes++
		}
	}

	if activeOrConnectingMandelboxes >= config.GetMandelboxLimitPerUser() {
		return false, nil
	}

	return true, nil
}

// sanitizeEmail tries to match an email to a general email regex
// and if it fails, returns an empty string. This is done because
// the email can be spoofed from the frontend.
func sanitizeEmail(email string) (string, error) {
	emailRegex := `(^[a-zA-Z0-9_.+-]+@[a-zA-Z0-9-]+\.[a-zA-Z0-9-.]+$)`

	match, err := regexp.Match(emailRegex, []byte(email))
	if err != nil {
		return "", fmt.Errorf("error sanitizing user email: %s", err)
	}

	if match {
		return email, nil
	} else {
		return "", nil
	}
}

// printSlice is a helper function to print a slice as a string of comma separated values.
// The string is truncated to the first n elements in the slice, to improve readability.
func printSlice[T constraints.Ordered](slice []T, n int) string {
	if len(slice) < n {
		n = len(slice)
	}

	var message string
	for i, v := range slice[:n] {
		if i+1 == n {
			message += fmt.Sprintf("%v", v)
		} else {
			message += fmt.Sprintf("%v, ", v)
		}
	}
	return message
}
