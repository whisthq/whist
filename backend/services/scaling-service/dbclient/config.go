package dbclient

import (
	"context"

	"github.com/hasura/go-graphql-client"
	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/subscriptions"
	"github.com/whisthq/whist/backend/services/utils"
)

// These functions are standalone (do not belong to a WhistDBClient implementation) because
// they are only used for populating configuration values before starting the scaling algorithm.

// GetDevConfigs will query the config database's `dev` table and parse the result as a map[string]string.
func GetDevConfigs(ctx context.Context, client subscriptions.WhistGraphQLClient) (map[string]string, error) {
	query := subscriptions.QueryDevConfigurations
	err := client.Query(ctx, &query, map[string]interface{}{})
	if err != nil {
		return nil, utils.MakeError("Failed to query config database for configuration values of env %v. Using default values instead. Err: %v", metadata.GetAppEnvironmentLowercase(), err)
	}

	if len(query.WhistConfigs) == 0 {
		return nil, utils.MakeError("Could not find dev configs on database")
	}

	// Convert to a map for easier manipulation
	configMap := make(map[string]string)
	for _, entry := range query.WhistConfigs {
		configMap[string(entry.Key)] = string(entry.Value)
	}

	return configMap, nil
}

// GetDevConfigs will query the config database's `staging` table and parse the result as a map[string]string.
func GetStagingConfigs(ctx context.Context, client subscriptions.WhistGraphQLClient) (map[string]string, error) {
	query := subscriptions.QueryStagingConfigurations
	err := client.Query(ctx, &query, map[string]interface{}{})
	if err != nil {
		return nil, utils.MakeError("Failed to query config database for configuration values of env %v. Using default values instead. Err: %v", metadata.GetAppEnvironmentLowercase(), err)
	}

	if len(query.WhistConfigs) == 0 {
		return nil, utils.MakeError("Could not find staging configs on database")
	}

	// Convert to a map for easier manipulation
	configMap := make(map[string]string)
	for _, entry := range query.WhistConfigs {
		configMap[string(entry.Key)] = string(entry.Value)
	}

	return configMap, nil
}

// GetDevConfigs will query the config database's `prod` table and parse the result as a map[string]string.
func GetProdConfigs(ctx context.Context, client subscriptions.WhistGraphQLClient) (map[string]string, error) {
	query := subscriptions.QueryProdConfigurations
	err := client.Query(ctx, &query, map[string]interface{}{})
	if err != nil {
		return nil, utils.MakeError("Failed to query config database for configuration values of env %v. Using default values instead. Err: %v", metadata.GetAppEnvironmentLowercase(), err)
	}

	if len(query.WhistConfigs) == 0 {
		return nil, utils.MakeError("Could not find prod configs on database")
	}

	// Convert to a map for easier manipulation
	configMap := make(map[string]string)
	for _, entry := range query.WhistConfigs {
		configMap[string(entry.Key)] = string(entry.Value)
	}

	return configMap, nil
}

// GetClientAppVersion will query the config database and get the only row of the `desktop_app_version` table.
func GetClientAppVersion(ctx context.Context, client subscriptions.WhistGraphQLClient) (subscriptions.ClientAppVersion, error) {
	// The version ID is always set to 1 on the database, since there
	// is only one row in the `desktop_client_app_version`.
	const versionID = 1

	query := subscriptions.QueryClientAppVersion
	err := client.Query(ctx, &query, map[string]interface{}{
		"id": graphql.Int(versionID),
	})
	if err != nil {
		return subscriptions.ClientAppVersion{}, utils.MakeError("Failed to query config database for client app version. Err: %v", err)
	}

	if len(query.WhistClientAppVersions) == 0 {
		return subscriptions.ClientAppVersion{}, utils.MakeError("Could not find client app version on config database")
	}

	version := subscriptions.ClientAppVersion{
		ID:                int(query.WhistClientAppVersions[0].ID),
		Major:             int(query.WhistClientAppVersions[0].Major),
		Minor:             int(query.WhistClientAppVersions[0].Minor),
		Micro:             int(query.WhistClientAppVersions[0].Micro),
		DevRC:             int(query.WhistClientAppVersions[0].DevRC),
		StagingRC:         int(query.WhistClientAppVersions[0].StagingRC),
		DevCommitHash:     string(query.WhistClientAppVersions[0].DevCommitHash),
		StagingCommitHash: string(query.WhistClientAppVersions[0].StagingCommitHash),
		ProdCommitHash:    string(query.WhistClientAppVersions[0].ProdCommitHash),
	}

	return version, nil
}
