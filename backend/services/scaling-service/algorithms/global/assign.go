package global

import (
	"context"
	"net"
	"time"

	"github.com/google/uuid"
	hashicorp "github.com/hashicorp/go-version"
	"github.com/whisthq/whist/backend/services/httputils"
	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/scaling-service/algorithms"
	"github.com/whisthq/whist/backend/services/scaling-service/algorithms/helpers"
	"github.com/whisthq/whist/backend/services/subscriptions"
	"github.com/whisthq/whist/backend/services/types"
	"github.com/whisthq/whist/backend/services/utils"
	logger "github.com/whisthq/whist/backend/services/whistlogger"
)

// MandelboxAssign is the action responsible for assigning an instance to a user,
// and scaling as necessary to satisfy demand.
func (s *GlobalScalingAlgorithm) MandelboxAssign(scalingCtx context.Context, event algorithms.ScalingEvent) error {
	logger.Infof("Starting mandelbox assign action for event: %v", event)
	defer logger.Infof("Finished mandelbox assign action for event: %v", event)

	// We want to verify if we have the desired capacity after assigning a mandelbox
	defer func() {
		err := s.VerifyCapacity(scalingCtx, event)
		if err != nil {
			logger.Errorf("Error verifying capacity on %v. Err: %v", event.Region, err)
		}
	}()

	mandelboxRequest := event.Data.(*httputils.MandelboxAssignRequest)

	// Note: we receive the email from the client, so its value should
	// not be trusted for anything else other than logging since
	// it can be spoofed. We sanitize the email before using to help mitigate
	// potential attacks.
	unsafeEmail, err := helpers.SanitizeEmail(mandelboxRequest.UserEmail)
	if err != nil {
		// err is already wrapped here
		return err
	}

	var (
		requestedRegions   = mandelboxRequest.Regions // This is a list of the regions requested by the frontend, in order of proximity.
		availableRegions   []string                   // List of regions that are enabled and match the regions in the request.
		unavailableRegions []string                   // List of regions that are not enabled but are present in the request.
		assignedInstance   subscriptions.Instance     // The instance that is assigned to the user.
	)

	// Populate availableRegions
	for _, requestedRegion := range requestedRegions {
		var regionFound bool
		for _, enabledRegion := range GetEnabledRegions() {
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
		if metadata.GetAppEnvironment() == metadata.EnvProd {
			logger.Errorf("User %s requested access to the following unavailable regions: %s. Trying to find instance on remaining available regions.", unsafeEmail, utils.PrintSlice(unavailableRegions))
		}
	}

	// The user requested access to only unavailable regions. The last resort is to default to us-east-1.
	if len(unavailableRegions) == len(requestedRegions) {
		if metadata.GetAppEnvironment() == metadata.EnvProd {
			logger.Errorf("User %s requested access to only unavailable regions: %s. Defaulting to us-east-1.", unsafeEmail, utils.PrintSlice(unavailableRegions))
		}
		availableRegions = []string{"us-east-1"}
	}

	if len(availableRegions) == 0 {
		err := utils.MakeError("")
		mandelboxRequest.ReturnResult(httputils.MandelboxAssignRequestResult{
			Error: REGION_NOT_ENABLED,
		}, err)
		return err
	}

	// This condition is to accomodate the worflow for developers of client_apps
	// to test their changes without needing to update the development database with
	// commit_hashes on their local machines.
	if metadata.IsLocalEnv() || mandelboxRequest.CommitHash == CLIENT_COMMIT_HASH_DEV_OVERRIDE {
		// Query for the latest image id
		imageResult, err := s.DBClient.QueryImage(scalingCtx, s.GraphQLClient, "AWS", event.Region) // TODO: set different provider when doing multi-cloud.
		if err != nil {
			return utils.MakeError("failed to query database for current image. Err: %v", err)
		}

		if len(imageResult) == 0 {
			return utils.MakeError("Image not found on %v.", event.Region)
		}

		mandelboxRequest.CommitHash = string(imageResult[0].ClientSHA)
	}

	// This is the "main" loop that does all the work and tries to find an instance for a user. first, it will iterate
	// over the list of regions provided on the request, and will query the database on each to return the list of
	// instances with capacity on the current region. Once it gets the instances, it will iterate over them and try
	// to find an instance with a matching commit hash. If it fails to do so, move on to the next region.
	for _, region := range availableRegions {
		logger.Infof("Trying to find instance for user %v in region %v, with commit hash %v. (client reported email %v, this value might not be accurate and is untrusted)",
			unsafeEmail, region, mandelboxRequest.CommitHash, unsafeEmail)

		instanceResult, err := s.DBClient.QueryInstanceWithCapacity(scalingCtx, s.GraphQLClient, region)
		if err != nil {
			return utils.MakeError("failed to query for instance with capacity. Err: %v", err)
		}

		if len(instanceResult) == 0 {
			logger.Warningf("Failed to find an instance in %v for commit hash %v. Trying on next region.", region, mandelboxRequest.CommitHash)
			continue
		}

		var instanceFound bool

		// Iterate over available instances, try to find one with a matching commit hash
		for i := range instanceResult {
			assignedInstance = subscriptions.Instance{
				ID:                string(instanceResult[i].ID),
				IPAddress:         string(instanceResult[i].IPAddress),
				Provider:          string(instanceResult[i].Provider),
				Region:            string(instanceResult[i].Region),
				ImageID:           string(instanceResult[i].ImageID),
				ClientSHA:         string(instanceResult[i].ClientSHA),
				Type:              string(instanceResult[i].Type),
				RemainingCapacity: int64(instanceResult[i].RemainingCapacity),
				Status:            string(instanceResult[i].Status),
				CreatedAt:         instanceResult[i].CreatedAt,
				UpdatedAt:         instanceResult[i].UpdatedAt,
			}

			if assignedInstance.ClientSHA == mandelboxRequest.CommitHash {
				logger.Infof("Found instance %v for user %v with commit hash %v.", assignedInstance.ID, mandelboxRequest.UserEmail, assignedInstance.ClientSHA)
				instanceFound = true
				break
			}
		}

		// Break of outer loop if instance was found. If no instance with
		// matching commit hash was found, move on to the next region.
		if instanceFound {
			break
		}
	}

	// No instances with capacity were found
	if assignedInstance == (subscriptions.Instance{}) {
		err := utils.MakeError("did not find an instance with capacity for user %v and commit hash %v.", mandelboxRequest.UserEmail, mandelboxRequest.CommitHash)
		mandelboxRequest.ReturnResult(httputils.MandelboxAssignRequestResult{
			Error: NO_INSTANCE_AVAILABLE,
		}, err)
		return err
	}

	var (
		// The parsed version from the config
		parsedFrontendVersion *hashicorp.Version
		// The parsed version from the request
		parsedRequestVersion *hashicorp.Version
		// whether the client app has an outdated version
		isOutdatedClient bool
	)

	// Get the version we keep locally for comparing the incoming request value.
	frontendVersion := getFrontendVersion()

	// Parse the version with the `hashicorp/go-version` package so we can compare.
	parsedFrontendVersion, err = hashicorp.NewVersion(frontendVersion)
	if err != nil {
		logger.Errorf("Failed parsing client app version from scaling algorithm config. Err: %v", err)
	}

	// Parse the version we got in the request.
	parsedRequestVersion, err = hashicorp.NewVersion(mandelboxRequest.Version)
	if err != nil {
		logger.Errorf("Failed parsing client app version from request. Err: %v", err)
	}

	// Compare the request version with the one from the config. If the
	// version from the request is less than the one we have locally, it
	// means the request comes from an outdated frontend application.
	if parsedFrontendVersion != nil && parsedRequestVersion != nil {
		isOutdatedClient = parsedRequestVersion.LessThan(parsedFrontendVersion)
	}

	// There are instances with capacity available, but none of them with the desired commit hash.
	// We only consider this error in cases when the client app has a version greater or equal than
	// the one in the config database. This is because when the client version is lesser (outdated client),
	// it will automatically update itself to the most recent version and send another request.
	if assignedInstance.ClientSHA != mandelboxRequest.CommitHash {
		var msg error
		if !isOutdatedClient {
			msg = utils.MakeError("found instance with capacity but it has a different commit hash %v than frontend with commit hash  %v", assignedInstance.ClientSHA, mandelboxRequest.CommitHash)
		}

		// Regardless if we log the error, its necessary to return an appropiate response.
		mandelboxRequest.ReturnResult(httputils.MandelboxAssignRequestResult{
			Error: COMMIT_HASH_MISMATCH,
		}, msg)
		return msg
	}

	mandelboxID, err := uuid.NewRandom()
	if err != nil {
		return utils.MakeError("failed to create a mandelbox id. Err: %v", err)
	}

	mandelboxesForDb := []subscriptions.Mandelbox{
		{
			ID:         types.MandelboxID(mandelboxID),
			App:        "CHROME",
			InstanceID: assignedInstance.ID,
			UserID:     mandelboxRequest.UserID,
			SessionID:  utils.Sprintf("%v", mandelboxRequest.SessionID),
			Status:     "ALLOCATED",
			CreatedAt:  time.Now(),
		},
	}

	// Allocate mandelbox on database, this will start the mandelbox inside the assigned instance
	affectedRows, err := s.DBClient.InsertMandelboxes(scalingCtx, s.GraphQLClient, mandelboxesForDb)
	if err != nil {
		return utils.MakeError("error while inserting mandelbox to database. Err: %v", err)
	}

	logger.Infof("Inserted %v rows to database.", affectedRows)

	if assignedInstance.RemainingCapacity <= 0 {
		// This should never happen, but we should consider
		// possible edge cases before updating the database.
		return utils.MakeError("instance with id %v has a remaning capacity less than or equal to 0.", assignedInstance.ID)
	}

	// Subtract 1 from the current instance capacity because we allocated a mandelbox
	updatedCapacity := assignedInstance.RemainingCapacity - 1
	assignedInstance.RemainingCapacity = updatedCapacity

	affectedRows, err = s.DBClient.UpdateInstance(scalingCtx, s.GraphQLClient, assignedInstance)
	if err != nil {
		return utils.MakeError("error while updating instance capacity on database. Err: %v", err)
	}

	logger.Infof("Updated %v rows in database.", affectedRows)

	// Parse IP address. The database uses the CIDR notation (192.0.2.0/24)
	// so we need to extract the address and send it to the frontend.
	ip, _, err := net.ParseCIDR(assignedInstance.IPAddress)
	if err != nil {
		return utils.MakeError("failed to parse IP address %v. Err: %v", assignedInstance.IPAddress, err)
	}

	// Return result to assign request
	mandelboxRequest.ReturnResult(httputils.MandelboxAssignRequestResult{
		IP:          ip.String(),
		MandelboxID: types.MandelboxID(mandelboxID),
	}, nil)

	return nil
}
