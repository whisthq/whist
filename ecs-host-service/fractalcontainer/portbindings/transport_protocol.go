package portbindings // import "github.com/fractal/fractal/ecs-host-service/fractalcontainer/portbinding"

import (
	"strings"

	logger "github.com/fractal/fractal/ecs-host-service/fractallogger"
)

// This block contains the two transport protocols (TCP and UDP) that we care
// about in container port bindings.
const (
	TransportProtocolTCP transportProtocol = "tcp"
	TransportProtocolUDP transportProtocol = "udp"
)

// A transportProtocol is either TCP or UDP. It is used for port mappings of containers.
type transportProtocol string

// UnmarshalJSON for transportProtocol determines whether to use TCP or UDP,
// setting TCP as the zero-value but treating other unrecognized values as
// errors
func (tp *transportProtocol) UnmarshalJSON(b []byte) error {
	if strings.ToLower(string(b)) == "null" {
		*tp = TransportProtocolTCP
		logger.Errorf("Unmarshalled nil transportProtocol as TCP")
		return nil
	}
	switch string(b) {
	case `"tcp"`:
		*tp = TransportProtocolTCP
	case `"udp"`:
		*tp = TransportProtocolUDP
	default:
		*tp = TransportProtocolTCP
		return logger.MakeError("TransportProtocol must be \"tcp\" or \"udp\"; Got " + string(b))
	}
	return nil
}

// MarshalJSON overrides the logic for JSON-encoding the transportProtocol type
func (tp transportProtocol) MarshalJSON() ([]byte, error) {
	return []byte(`"` + string(tp) + `"`), nil
}

// NewTransportProtocol returns a transportProtocol from a string in the task
func NewTransportProtocol(protocol string) (transportProtocol, error) {
	switch protocol {
	case string(TransportProtocolTCP):
		return TransportProtocolTCP, nil
	case string(TransportProtocolUDP):
		return TransportProtocolUDP, nil
	default:
		return TransportProtocolTCP, logger.MakeError(protocol + " is not a recognized transport protocol")
	}
}
