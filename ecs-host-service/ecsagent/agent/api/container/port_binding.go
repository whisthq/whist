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

package container

import (
	"strconv"

	fractalportbindings "github.com/fractal/fractal/ecs-host-service/fractalcontainer/portbindings"

	"github.com/docker/go-connections/nat"
	apierrors "github.com/fractal/fractal/ecs-host-service/ecsagent/agent/api/errors"
)

const (
	// UnrecognizedTransportProtocolErrorName is an error where the protocol of the binding is invalid
	UnrecognizedTransportProtocolErrorName = "UnrecognizedTransportProtocol"
	// UnparseablePortErrorName is an error where the port configuration is invalid
	UnparseablePortErrorName = "UnparsablePort"
)

// PortBindingFromDockerPortBinding constructs a fractalportbindings.PortBinding slice from a docker
// NetworkSettings.Ports map.
func PortBindingFromDockerPortBinding(dockerPortBindings nat.PortMap) ([]fractalportbindings.PortBinding, apierrors.NamedError) {
	portBindings := make([]fractalportbindings.PortBinding, 0, len(dockerPortBindings))

	for port, bindings := range dockerPortBindings {
		containerPort, err := nat.ParsePort(port.Port())
		if err != nil {
			return nil, &apierrors.DefaultNamedError{Name: UnparseablePortErrorName, Err: "Error parsing docker port as int " + err.Error()}
		}
		protocol, err := fractalportbindings.NewTransportProtocol(port.Proto())
		if err != nil {
			return nil, &apierrors.DefaultNamedError{Name: UnrecognizedTransportProtocolErrorName, Err: err.Error()}
		}

		for _, binding := range bindings {
			hostPort, err := strconv.Atoi(binding.HostPort)
			if err != nil {
				return nil, &apierrors.DefaultNamedError{Name: UnparseablePortErrorName, Err: "Error parsing port binding as int " + err.Error()}
			}
			portBindings = append(portBindings, fractalportbindings.PortBinding{
				ContainerPort: uint16(containerPort),
				HostPort:      uint16(hostPort),
				BindIP:        binding.HostIP,
				Protocol:      protocol,
			})
		}
	}
	return portBindings, nil
}
