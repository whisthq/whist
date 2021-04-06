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

// Package engine contains code for interacting with container-running backends and handling events from them.
// It supports Docker as the sole task engine type.
package engine

import (
	"github.com/fractal/fractal/ecs-host-service/ecsagent/agent/config"
	"github.com/fractal/fractal/ecs-host-service/ecsagent/agent/containermetadata"
	"github.com/fractal/fractal/ecs-host-service/ecsagent/agent/credentials"
	"github.com/fractal/fractal/ecs-host-service/ecsagent/agent/dockerclient/dockerapi"
	"github.com/fractal/fractal/ecs-host-service/ecsagent/agent/engine/dockerstate"
	"github.com/fractal/fractal/ecs-host-service/ecsagent/agent/engine/execcmd"
	"github.com/fractal/fractal/ecs-host-service/ecsagent/agent/eventstream"
	"github.com/fractal/fractal/ecs-host-service/ecsagent/agent/taskresource"
)

// NewTaskEngine returns a default TaskEngine
func NewTaskEngine(cfg *config.Config, client dockerapi.DockerClient,
	credentialsManager credentials.Manager,
	containerChangeEventStream *eventstream.EventStream,
	imageManager ImageManager, state dockerstate.TaskEngineState,
	metadataManager containermetadata.Manager,
	resourceFields *taskresource.ResourceFields,
	execCmdMgr execcmd.Manager) TaskEngine {

	taskEngine := NewDockerTaskEngine(cfg, client, credentialsManager,
		containerChangeEventStream, imageManager,
		state, metadataManager, resourceFields, execCmdMgr)

	return taskEngine
}
