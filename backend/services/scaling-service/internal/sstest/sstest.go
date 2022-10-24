// Copyright (c) 2022 Whist Technologies, Inc.

// Package sstest provides common testing utilities.
package sstest

import (
	"context"
	"encoding/json"
	"fmt"
	"reflect"
	"strconv"
	"strings"
	"unsafe"

	"github.com/hasura/go-graphql-client"
	"github.com/whisthq/whist/backend/services/subscriptions"
)

// The TestClient type implements the subscriptions.WhistGraphQLClient
// interface. We use it to mock subscriptions.GraphQLClient.
type TestClient struct {
	TargetFreeMandelboxes map[string]int
	MandelboxLimit        int32
}

// Initialize is part of the subscriptions.WhistGraphQLClient interface.
func (*TestClient) Initialize(bool) error {
	return nil
}

// Mutate is part of the subscriptions.WhistGraphQLClient interface.
func (*TestClient) Mutate(context.Context, subscriptions.GraphQLQuery, map[string]interface{}) error {
	return nil
}

// Query is part of the subscriptions.WhistGraphQLClient interface. This
// implementation populates the query struct with mock region data.
func (c *TestClient) Query(_ context.Context, q subscriptions.GraphQLQuery, _ map[string]interface{}) error {
	type entry struct {
		Key   graphql.String `graphql:"key"`
		Value graphql.String `graphql:"value"`
	}

	n := len(c.TargetFreeMandelboxes)
	regions := make([]string, 0, n)

	// There are n + 2 entries in the configuration table because one entry is
	// ENABLED_REGIONS, one is MANDELBOX_LIMIT_PER_USER, and there is one entry
	// for each of the n enabled regions that specifies the desired number of
	// free Mandelboxes in that region.
	configs := make([]entry, 0, n+2)

	for region, count := range c.TargetFreeMandelboxes {
		regions = append(regions, region)

		suffix := strings.ToUpper(strings.ReplaceAll(region, "-", "_"))
		key := graphql.String(fmt.Sprintf("DESIRED_FREE_MANDELBOXES_%s", suffix))
		value := graphql.String(strconv.FormatInt(int64(count), 10))
		configs = append(configs, entry{key, value})
	}

	regionJson, err := json.Marshal(regions)

	if err != nil {
		return err
	}

	regionJsonString := graphql.String(regionJson)
	limit := graphql.String(strconv.FormatInt(int64(c.MandelboxLimit), 10))

	configs = append(configs, entry{"ENABLED_REGIONS", regionJsonString})
	configs = append(configs, entry{"MANDELBOX_LIMIT_PER_USER", limit})

	var config reflect.Value

	// If reflect.ValueOf(q) is a Pointer, reflect.Indirect() will dereference it,
	// otherwise it will return reflect.ValueOf(q). We do this instead of
	// reflect.TypeOf(q).Elem() just in case reflect.TypeOf(q) is not a Pointer.
	// In that case, Elem() would panic.
	ty := reflect.Indirect(reflect.ValueOf(q)).Type()

	switch ty {
	case reflect.TypeOf(subscriptions.QueryFrontendVersion):
		config = reflect.Indirect(reflect.ValueOf(q)).FieldByName("WhistFrontendVersions")
		entry := subscriptions.WhistFrontendVersion{
			ID:    1,
			Major: 1,
			Minor: 0,
			Micro: 0,
		}
		config.Set(reflect.Append(config, reflect.NewAt(reflect.TypeOf(entry),
			unsafe.Pointer(&entry)).Elem()))

		// Use different cases with fallthrough statements rather than using a
		// single case to avoid having one super long line.
	case reflect.TypeOf(subscriptions.QueryDevConfigurations):
		fallthrough
	case reflect.TypeOf(subscriptions.QueryStagingConfigurations):
		fallthrough
	case reflect.TypeOf(subscriptions.QueryProdConfigurations):
		config = reflect.Indirect(reflect.ValueOf(q)).FieldByName("WhistConfigs")

		for _, entry := range configs {
			config.Set(reflect.Append(config, reflect.NewAt(reflect.TypeOf(entry),
				unsafe.Pointer(&entry)).Elem()))
		}
	default:
		return fmt.Errorf("not implemented: %v", ty)
	}

	return nil
}
