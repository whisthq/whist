package portbindings // import "github.com/fractal/fractal/host-service/mandelbox/portbinding"

import (
	"strings"

	logger "github.com/fractal/fractal/host-service/fractallogger"
	"github.com/fractal/fractal/host-service/utils"
)

// This block contains the two transport protocols (TCP and UDP) that we care
// about in mandelbox port bindings.
const (
	TransportProtocolTCP TransportProtocol = "tcp"
	TransportProtocolUDP TransportProtocol = "udp"
)

// A TransportProtocol is either TCP or UDP. It is used for port mappings of
// mandelboxes.
type TransportProtocol string

// UnmarshalJSON for transportProtocol determines whether to use TCP or UDP,
// setting TCP as the zero-value but treating other unrecognized values as
// errors
func (tp *TransportProtocol) UnmarshalJSON(b []byte) error {
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
		return utils.MakeError("TransportProtocol must be \"tcp\" or \"udp\"; Got " + string(b))
	}
	return nil
}

// MarshalJSON overrides the logic for JSON-encoding the transportProtocol type
func (tp TransportProtocol) MarshalJSON() ([]byte, error) {
	return []byte(`"` + string(tp) + `"`), nil
}

// NewTransportProtocol returns a transportProtocol from a string in the task
func NewTransportProtocol(protocol string) (TransportProtocol, error) {
	switch protocol {
	case string(TransportProtocolTCP):
		return TransportProtocolTCP, nil
	case string(TransportProtocolUDP):
		return TransportProtocolUDP, nil
	default:
		return TransportProtocolTCP, utils.MakeError(protocol + " is not a recognized transport protocol")
	}
}
