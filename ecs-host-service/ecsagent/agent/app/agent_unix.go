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

package app

import (
	"fmt"
	"os"

	asmfactory "github.com/fractal/fractal/ecs-host-service/ecsagent/agent/asm/factory"
	"github.com/fractal/fractal/ecs-host-service/ecsagent/agent/config"
	"github.com/fractal/fractal/ecs-host-service/ecsagent/agent/credentials"
	"github.com/fractal/fractal/ecs-host-service/ecsagent/agent/ecs_client/model/ecs"
	"github.com/fractal/fractal/ecs-host-service/ecsagent/agent/engine/dockerstate"
	"github.com/fractal/fractal/ecs-host-service/ecsagent/agent/eni/udevwrapper"
	"github.com/fractal/fractal/ecs-host-service/ecsagent/agent/eni/watcher"
	"github.com/fractal/fractal/ecs-host-service/ecsagent/agent/gpu"
	ssmfactory "github.com/fractal/fractal/ecs-host-service/ecsagent/agent/ssm/factory"

	"github.com/aws/aws-sdk-go/aws/awserr"
	"github.com/cihub/seelog"
	"github.com/fractal/fractal/ecs-host-service/ecsagent/agent/statechange"
	"github.com/fractal/fractal/ecs-host-service/ecsagent/agent/taskresource"
	cgroup "github.com/fractal/fractal/ecs-host-service/ecsagent/agent/taskresource/cgroup/control"
	"github.com/fractal/fractal/ecs-host-service/ecsagent/agent/utils/ioutilwrapper"
	"github.com/pkg/errors"
)

// initPID defines the process identifier for the init process
const initPID = 1

// startWindowsService is not supported on Linux
func (agent *ecsAgent) startWindowsService() int {
	seelog.Error("Windows Services are not supported on Linux")
	return 1
}

var getPid = os.Getpid

// setVPCSubnet sets the vpc and subnet ids for the agent by querying the
// instance metadata service
func (agent *ecsAgent) setVPCSubnet() (error, bool) {
	mac, err := agent.ec2MetadataClient.PrimaryENIMAC()
	if err != nil {
		return fmt.Errorf("unable to get mac address of instance's primary ENI from instance metadata: %v", err), false
	}

	vpcID, err := agent.ec2MetadataClient.VPCID(mac)
	if err != nil {
		if isInstanceLaunchedInVPC(err) {
			return fmt.Errorf("unable to get vpc id from instance metadata: %v", err), true
		}
		return instanceNotLaunchedInVPCError, false
	}

	subnetID, err := agent.ec2MetadataClient.SubnetID(mac)
	if err != nil {
		return fmt.Errorf("unable to get subnet id from instance metadata: %v", err), false
	}
	agent.vpc = vpcID
	agent.subnet = subnetID
	agent.mac = mac
	return nil, false
}

// isInstanceLaunchedInVPC returns false when the awserr returned is an EC2MetadataError
// when querying the vpc id from instance metadata
func isInstanceLaunchedInVPC(err error) bool {
	if aerr, ok := err.(awserr.Error); ok &&
		aerr.Code() == "EC2MetadataError" {
		return false
	}
	return true
}

// startUdevWatcher starts the udev monitor and the watcher for receiving
// notifications from the monitor
func (agent *ecsAgent) startUdevWatcher(state dockerstate.TaskEngineState, stateChangeEvents chan<- statechange.Event) error {
	seelog.Debug("Setting up ENI Watcher")
	if agent.udevMonitor == nil {
		monitor, err := udevwrapper.New()
		if err != nil {
			return errors.Wrapf(err, "unable to create udev monitor")
		}
		agent.udevMonitor = monitor

		// Create Watcher
		eniWatcher := watcher.New(agent.ctx, agent.mac, agent.udevMonitor, state, stateChangeEvents)
		if err := eniWatcher.Init(); err != nil {
			return errors.Wrapf(err, "unable to initialize eni watcher")
		}
		go eniWatcher.Start()
	}
	return nil
}

func contains(capabilities []string, capability string) bool {
	for _, cap := range capabilities {
		if cap == capability {
			return true
		}
	}

	return false
}

// initializeResourceFields exists mainly for testing doStart() to use mock Control
// object
func (agent *ecsAgent) initializeResourceFields(credentialsManager credentials.Manager) {
	agent.resourceFields = &taskresource.ResourceFields{
		Control: cgroup.New(),
		ResourceFieldsCommon: &taskresource.ResourceFieldsCommon{
			IOUtil:             ioutilwrapper.NewIOUtil(),
			ASMClientCreator:   asmfactory.NewClientCreator(),
			SSMClientCreator:   ssmfactory.NewSSMClientCreator(),
			CredentialsManager: credentialsManager,
			EC2InstanceID:      agent.getEC2InstanceID(),
		},
		Ctx:              agent.ctx,
		DockerClient:     agent.dockerClient,
		NvidiaGPUManager: gpu.NewNvidiaGPUManager(),
	}
}

func (agent *ecsAgent) cgroupInit() error {
	err := agent.resourceFields.Control.Init()
	// When task CPU and memory limits are enabled, all tasks are placed
	// under the '/ecs' cgroup root.
	if err == nil {
		return nil
	}
	if agent.cfg.TaskCPUMemLimit.Value == config.ExplicitlyEnabled {
		return errors.Wrapf(err, "unable to setup '/ecs' cgroup")
	}
	seelog.Warnf("Disabling TaskCPUMemLimit because agent is unabled to setup '/ecs' cgroup: %v", err)
	agent.cfg.TaskCPUMemLimit.Value = config.ExplicitlyDisabled
	return nil
}

func (agent *ecsAgent) initializeGPUManager() error {
	if agent.resourceFields != nil && agent.resourceFields.NvidiaGPUManager != nil {
		return agent.resourceFields.NvidiaGPUManager.Initialize()
	}
	return nil
}

func (agent *ecsAgent) getPlatformDevices() []*ecs.PlatformDevice {
	if agent.cfg.GPUSupportEnabled {
		if agent.resourceFields != nil && agent.resourceFields.NvidiaGPUManager != nil {
			return agent.resourceFields.NvidiaGPUManager.GetDevices()
		}
	}
	return nil
}

func (agent *ecsAgent) loadPauseContainer() error {
	// Load the pause container's image from the 'disk'
	_, err := agent.pauseLoader.LoadImage(agent.ctx, agent.cfg, agent.dockerClient)

	return err
}
