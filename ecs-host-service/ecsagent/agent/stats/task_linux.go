// +build linux

// Copyright Amazon.com Inc. or its affiliates. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License"). You may
// not use this file except in compliance with the License. A copy of the
// License is located at
//
//	http://aws.amazon.com/apache2.0/
//
// or in the "license" file accompanying this file. This file is distributed
// on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
// express or implied. See the License for the specific language governing
// permissions and limitations under the License.

package stats

import (
	"context"
	"time"

	apitaskstatus "github.com/fractal/fractal/ecs-host-service/ecsagent/agent/api/task/status"
	"github.com/fractal/fractal/ecs-host-service/ecsagent/agent/eni/netlinkwrapper"
	"github.com/fractal/fractal/ecs-host-service/ecsagent/agent/stats/resolver"
	"github.com/fractal/fractal/ecs-host-service/ecsagent/agent/utils/nswrapper"

	dockerstats "github.com/docker/docker/api/types"
	netlinklib "github.com/vishvananda/netlink"
)

const (
	// linkTypeDevice defines the string that's expected to be the output of
	// netlink.Link.Type() method for netlink.Device type.
	linkTypeDevice = "device"
	linkTypeVlan   = "vlan"
	// encapTypeLoopback defines the string that's set for the link.Attrs.EncapType
	// field for localhost devices. The EncapType field defines the link
	// encapsulation method. For localhost, it's set to "loopback".
	encapTypeLoopback = "loopback"
)

// StatsTask abstracts methods to gather and aggregate network data for a task. Used only for AWSVPC mode.
type StatsTask struct {
	StatsQueue            *Queue
	TaskMetadata          *TaskMetadata
	Ctx                   context.Context
	Cancel                context.CancelFunc
	Resolver              resolver.ContainerMetadataResolver
	nswrapperinterface    nswrapper.NS
	netlinkinterface      netlinkwrapper.NetLink
	metricPublishInterval time.Duration
}

func newStatsTaskContainer(taskARN string, containerPID string, numberOfContainers int,
	resolver resolver.ContainerMetadataResolver, publishInterval time.Duration) (*StatsTask, error) {
	nsAgent := nswrapper.NewNS()
	netlinkclient := netlinkwrapper.New()

	ctx, cancel := context.WithCancel(context.Background())
	return &StatsTask{
		TaskMetadata: &TaskMetadata{
			TaskArn:          taskARN,
			ContainerPID:     containerPID,
			NumberContainers: numberOfContainers,
		},
		Ctx:                   ctx,
		Cancel:                cancel,
		Resolver:              resolver,
		netlinkinterface:      netlinkclient,
		nswrapperinterface:    nsAgent,
		metricPublishInterval: publishInterval,
	}, nil
}

func (taskStat *StatsTask) terminal() (bool, error) {
	resolvedTask, err := taskStat.Resolver.ResolveTaskByARN(taskStat.TaskMetadata.TaskArn)
	if err != nil {
		return false, err
	}
	return resolvedTask.GetKnownStatus() == apitaskstatus.TaskStopped, nil
}

func getDevicesList(linkList []netlinklib.Link) []string {
	var deviceNames []string
	for _, link := range linkList {
		if link.Type() != linkTypeDevice && link.Type() != linkTypeVlan {
			// We only care about netlink.Device/netlink.Vlan types. Ignore other link types.
			continue
		}
		if link.Attrs().EncapType == encapTypeLoopback {
			// Ignore localhost
			continue
		}
		deviceNames = append(deviceNames, link.Attrs().Name)
	}
	return deviceNames
}

func linkStatsToDockerStats(netLinkStats *netlinklib.LinkStatistics, numberOfContainers uint64) dockerstats.NetworkStats {
	networkStats := dockerstats.NetworkStats{
		RxBytes:   netLinkStats.RxBytes / numberOfContainers,
		RxPackets: netLinkStats.RxPackets / numberOfContainers,
		RxErrors:  netLinkStats.RxErrors / numberOfContainers,
		RxDropped: netLinkStats.RxDropped / numberOfContainers,
		TxBytes:   netLinkStats.TxBytes / numberOfContainers,
		TxPackets: netLinkStats.TxPackets / numberOfContainers,
		TxErrors:  netLinkStats.TxErrors / numberOfContainers,
		TxDropped: netLinkStats.TxDropped / numberOfContainers,
	}
	return networkStats
}
