// Copyright (c) 2022 Whist Technologies, Inc.

package internal

// The Args struct stores values passed on the command line.
type Args struct {
	// GraphQLEndpoint is the URL of the database's Hasura engine.
	GraphQLEndpoint string

	// ConfigGraphQLEndpoint is the URL of the configuration database's Hasura
	// engine.
	ConfigGraphQLEndpoint string
}
