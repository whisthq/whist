package fractaltypes

import (
	ecsengine "github.com/fractal/ecs-agent/agent/engine"
)

// UinputDeviceMapping and PortBindings are the type we manipulate in the
// higher-level packages of the host service to avoid having to import
// ecs-agent packages into the main package (for modularity).
type UinputDeviceMapping ecsengine.FractalUinputDeviceMapping
type PortBindings ecsengine.FractalPortBinding
