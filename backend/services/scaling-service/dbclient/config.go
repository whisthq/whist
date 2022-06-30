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
		return nil, utils.MakeError("failed to query config database for %s values: %s", metadata.GetAppEnvironmentLowercase(), err)
	}

	if len(query.WhistConfigs) == 0 {
		return nil, utils.MakeError("could not find dev configs on database")
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
		return nil, utils.MakeError("failed to query config database for %s values: %s", metadata.GetAppEnvironmentLowercase(), err)
	}

	if len(query.WhistConfigs) == 0 {
		return nil, utils.MakeError("could not find staging configs on database")
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
		return nil, utils.MakeError("failed to query config database for %s values: %s", metadata.GetAppEnvironmentLowercase(), err)
	}

	if len(query.WhistConfigs) == 0 {
		return nil, utils.MakeError("could not find prod configs on database")
	}

	// Convert to a map for easier manipulation
	configMap := make(map[string]string)
	for _, entry := range query.WhistConfigs {
		configMap[string(entry.Key)] = string(entry.Value)
	}

	return configMap, nil
}

// GetFrontendVersion will query the config database and get the only row of the `desktop_app_version` table.
func GetFrontendVersion(ctx context.Context, client subscriptions.WhistGraphQLClient) (subscriptions.FrontendVersion, error) {
	// The version ID is always set to 1 on the database, since there
	// is only one row in the the config DB version table.
	const versionID = 1

	query := subscriptions.QueryFrontendVersion
	err := client.Query(ctx, &query, map[string]interface{}{
		"id": graphql.Int(versionID),
	})
	if err != nil {
		return subscriptions.FrontendVersion{}, utils.MakeError("failed to query config database for frontend version: %s", err)
	}

	if len(query.WhistFrontendVersions) == 0 {
		return subscriptions.FrontendVersion{}, utils.MakeError("could not find frontend version on config database")
	}

	version := subscriptions.FrontendVersion{
		ID:                int(query.WhistFrontendVersions[0].ID),
		Major:             int(query.WhistFrontendVersions[0].Major),
		Minor:             int(query.WhistFrontendVersions[0].Minor),
		Micro:             int(query.WhistFrontendVersions[0].Micro),
		DevRC:             int(query.WhistFrontendVersions[0].DevRC),
		StagingRC:         int(query.WhistFrontendVersions[0].StagingRC),
		DevCommitHash:     string(query.WhistFrontendVersions[0].DevCommitHash),
		StagingCommitHash: string(query.WhistFrontendVersions[0].StagingCommitHash),
		ProdCommitHash:    string(query.WhistFrontendVersions[0].ProdCommitHash),
	}

	return version, nil
}
