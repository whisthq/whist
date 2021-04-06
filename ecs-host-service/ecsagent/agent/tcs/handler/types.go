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

package tcshandler

import (
	"context"
	"sync"

	"github.com/fractal/fractal/ecs-host-service/ecsagent/agent/api"
	"github.com/fractal/fractal/ecs-host-service/ecsagent/agent/config"
	"github.com/fractal/fractal/ecs-host-service/ecsagent/agent/engine"
	"github.com/fractal/fractal/ecs-host-service/ecsagent/agent/eventstream"
	"github.com/fractal/fractal/ecs-host-service/ecsagent/agent/stats"
	"github.com/fractal/fractal/ecs-host-service/ecsagent/agent/utils/ttime"
	"github.com/aws/aws-sdk-go/aws/credentials"
	"github.com/pkg/errors"
)

// TelemetrySessionParams contains all the parameters required to start a tcs
// session
type TelemetrySessionParams struct {
	Ctx                           context.Context
	ContainerInstanceArn          string
	CredentialProvider            *credentials.Credentials
	Cfg                           *config.Config
	DeregisterInstanceEventStream *eventstream.EventStream
	AcceptInvalidCert             bool
	ECSClient                     api.ECSClient
	TaskEngine                    engine.TaskEngine
	StatsEngine                   *stats.DockerStatsEngine
	_time                         ttime.Time
	_timeOnce                     sync.Once
}

func (params *TelemetrySessionParams) time() ttime.Time {
	params._timeOnce.Do(func() {
		if params._time == nil {
			params._time = &ttime.DefaultTime{}
		}
	})
	return params._time
}

func (params *TelemetrySessionParams) isContainerHealthMetricsDisabled() (bool, error) {
	if params.Cfg != nil {
		return params.Cfg.DisableMetrics.Enabled() && params.Cfg.DisableDockerHealthCheck.Enabled(), nil
	}
	return false, errors.New("Config is empty in the tcs session parameter")
}
