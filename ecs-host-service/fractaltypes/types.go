package fractaltypes

import (
	ecsengine "github.com/fractal/ecs-agent/agent/engine"
)

// UinputDeviceMapping and PortBindings are the type we manipulate in the
// higher-level packages of the host service to avoid having to import
// ecs-agent packages into the main package (for modularity).

// A UinputDeviceMapping represents uinput devices and their paths on the host
// and inside a given container.
type UinputDeviceMapping ecsengine.FractalUinputDeviceMapping

// A PortBinding represents a single port that is bound inside a container to a
// port with the same protocol on the host.
type PortBinding ecsengine.FractalPortBinding
