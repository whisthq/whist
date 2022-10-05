// Copyright (c) 2022 Whist Technologies, Inc.

package main

import (
	"context"
	"reflect"
	"time"

	"github.com/whisthq/whist/backend/services/scaling-service/dbclient"
	"github.com/whisthq/whist/backend/services/scaling-service/hosts"
	"github.com/whisthq/whist/backend/services/subscriptions"
	logger "github.com/whisthq/whist/backend/services/whistlogger"
)

var db dbclient.DBClient

// do marks all unresponsive instances as TERMINATING in the database before
// subsequently terminating them and finally removing them from the database
// altogether.
func do(ctx context.Context, client subscriptions.WhistGraphQLClient, h hosts.HostHandler) {
	region := h.GetRegion()
	maxAge := time.Now().Add(-150 * time.Second)
	ids, err := db.LockBrokenInstances(ctx, client, region, maxAge)

	if err != nil {
		logger.Errorf("failed to mark unresponsive instances as TERMINATING in "+
			"region %s: %s", region, err)
		return
	} else if len(ids) < 1 {
		logger.Debugf("Didn't find any unresponsive instances in region %s",
			region)
		return
	}

	if err := h.SpinDownInstances(ctx, ids); err != nil {
		logger.Errorf("failed to terminate unresponsive instances in region %s:"+
			"%s", region, err)
		logger.Errorf("please verify that the instances in %s with the following "+
			"instance IDs have been terminated and then remove them from the "+
			"database: %s", region, ids)
		return
	}

	deleted, err := db.TerminateLockedInstances(ctx, client, region, ids)

	if err != nil {
		logger.Errorf("failed to remove unresponsive instances in %s from the "+
			"database: %s", region, err)
		return
	}

	if !reflect.DeepEqual(deleted, ids) {
		logger.Errorf("some %s instance rows were not deleted: requested "+
			"%v, got %v", region, ids, deleted)
	} else {
		logger.Info("Successfully removed the following unresponsive instance "+
			"rows from the database for %s:", region, deleted)
	}
}
