// Copyright (c) 2022 Whist Technologies, Inc.

package internal

import "context"

// The Config type stores application configuration values.
type Config struct {
}

// PopulateConfig populates the configuration structure pointed to by cfg with
// values from the configuration database.
func PopulateConfig(q Querier, cfg *Config) error {
	// TODO(owen): Construct query.
	var query struct{}

	if err := q.Query(context.TODO(), &query, nil); err != nil {
		return err
	}

	// TODO(owen): Populate cfg with values from the configuration database.

	return nil
}

// ValidateConfig returns an error if cfg contains an invalid combination of
// configuration values. It should be called after populating a configuration
// struct.
func ValidateConfig(cfg Config) error {
	return nil
}
