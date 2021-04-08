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

package taskresource

import (
	asmfactory "github.com/fractal/fractal/ecs-host-service/ecsagent/agent/asm/factory"
	"github.com/fractal/fractal/ecs-host-service/ecsagent/agent/credentials"
	fsxfactory "github.com/fractal/fractal/ecs-host-service/ecsagent/agent/fsx/factory"
	ssmfactory "github.com/fractal/fractal/ecs-host-service/ecsagent/agent/ssm/factory"
	"github.com/fractal/fractal/ecs-host-service/ecsagent/agent/utils/ioutilwrapper"
)

type ResourceFieldsCommon struct {
	IOUtil             ioutilwrapper.IOUtil
	ASMClientCreator   asmfactory.ClientCreator
	SSMClientCreator   ssmfactory.SSMClientCreator
	FSxClientCreator   fsxfactory.FSxClientCreator
	CredentialsManager credentials.Manager
	EC2InstanceID      string
}
