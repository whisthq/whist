package portbindings

import (
	"bytes"
	"testing"
)

func TestUnmarshalJSONEmpty(t *testing.T) {
	tp := TransportProtocolUDP

	// Pass empty byte list
	err := (&tp).UnmarshalJSON([]byte{})

	if tp != TransportProtocolTCP {
		t.Fatalf("error unmarshalJSON with empty byte list. Expected tp=%v but got %v", TransportProtocolTCP, tp)
	}

	if err == nil {
		t.Fatalf("error unmarshalJSON with empty byte list. Expected err but got nil")
	}
}

func TestUnmarshalJSONNull(t *testing.T) {
	tp := TransportProtocolUDP

	// Pass null string as byte list
	err := (&tp).UnmarshalJSON([]byte("null"))

	if tp != TransportProtocolTCP {
		t.Fatalf("error unmarshalJSON with 'null' byte list. Expected tp=%v but got %v", TransportProtocolTCP, tp)
	}

	if err != nil {
		t.Fatalf("error unmarshalJSON with 'null' byte list. Error: %v", err)
	}
}

func TestUnmarshalJSONTcp(t *testing.T) {
	tp := TransportProtocolUDP

	// Pass tcp as byte list
	err := (&tp).UnmarshalJSON([]byte(`"tcp"`))

	if err != nil {
		t.Fatalf("error unmarshalJSON with 'tcp' byte list. Error: %v", err)
	}

	if tp != TransportProtocolTCP {
		t.Fatalf("error unmarshalJSON with 'tcp' byte list. Expected tp=%v but got %v", TransportProtocolTCP, tp)
	}
}

func TestUnmarshalJSONUdp(t *testing.T) {
	tp := TransportProtocolTCP
	// Pass udp as byte list
	err := (&tp).UnmarshalJSON([]byte(`"udp"`))

	if err != nil {
		t.Fatalf("error unmarshalJSON with 'udp' byte list. Error: %v", err)
	}

	if tp != TransportProtocolUDP {
		t.Fatalf("error unmarshalJSON with 'udp' byte list. Expected tp=%v but got %v", TransportProtocolTCP, tp)
	}
}

func TestMarshalJSON(t *testing.T) {
	tp := TransportProtocolTCP

	// Will get byte version of tp
	res, err := (&tp).MarshalJSON()
	if err != nil {
		t.Fatalf("error marshalJSON: %v", err)
	}

	if !bytes.Equal(res, []byte(`"tcp"`)) {
		t.Fatalf("error marshalJSON. Expected %v but got %v", `"tcp"`, string(res))
	}
}

func TestNewTransportProtocolTcp(t *testing.T) {
	protocol, err := NewTransportProtocol("tcp")
	if err != nil {
		t.Fatalf("error with new transport protocol tcp: %v", err)
	}

	if protocol != TransportProtocolTCP {
		t.Fatalf("error with new transport protocol tcp. Expected %v but got %v", TransportProtocolTCP, protocol)
	}
}

func TestNewTransportProtocolUdp(t *testing.T) {
	protocol, err := NewTransportProtocol("udp")
	if err != nil {
		t.Fatalf("error with new transport protocol udp: %v", err)
	}

	if protocol != TransportProtocolUDP {
		t.Fatalf("error with new transport protocol udp. Expected %v but got %v", TransportProtocolUDP, protocol)
	}
}

func TestNewTransportProtocolInvalid(t *testing.T) {
	protocol, err := NewTransportProtocol("testTransportProtocol")
	if protocol != TransportProtocolTCP {
		t.Fatalf("error with new transport protocol invalid. Expected %v but got %v", TransportProtocolTCP, protocol)
	}

	if err == nil {
		t.Fatalf("error with new transport protocol invalid. Expected err but got nil")
	}
}
