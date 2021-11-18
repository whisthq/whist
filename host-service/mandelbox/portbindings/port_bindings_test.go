package portbindings

import (
	"fmt"
	"testing"
)


func TestAllowedRangeMinInclusive(t *testing.T) {
	// MinAllowedPort is inclusive and should be in the range
	if !isInAllowedRange(MinAllowedPort) {
		t.Fatalf("error checking in allowed range Port as inclusive min was not allowed: %v", MinAllowedPort)
	}
}

func TestNotAllowedRangeMaxExclusive(t *testing.T) {
	// MaxAllowedPort is exclusive and should not be in the range
	if isInAllowedRange(MaxAllowedPort) {
		t.Fatalf("error checking in allowed range Port as exclusive max was allowed: %v", MaxAllowedPort)
	}
}

func TestNotAllowedRangeOutsideMinMax(t *testing.T) {
	// Anything greater than MaxAllowedPort should not be in the allowed range
	moreThanMaxAllowedPort := uint16(MaxAllowedPort + 1)
	if isInAllowedRange(moreThanMaxAllowedPort) {
		t.Fatalf("error checking in allowed range Port as value more than exclusive max was allowed: %v", moreThanMaxAllowedPort)
	}

	// MinAllowedPort is the lowest bound and anything below should return false
	lessThanMinAllowedPort := uint16(MinAllowedPort - 1)
	if isInAllowedRange(lessThanMinAllowedPort) {
		t.Fatalf("error checking in allowed range Port as value less than inclusive was allowed: %v", lessThanMinAllowedPort)
	}
}

func TestAllowedRangeMaxMinusOne(t *testing.T) {
	// MaxAllowedPort is exclusive meaning that one less is allowed
	oneLessThanMaxAllowedPort := uint16(MaxAllowedPort - 1)
	if !isInAllowedRange(oneLessThanMaxAllowedPort) {
		t.Fatalf("error checking in allowed range port as one less than exclusive max was not allowed: %v", oneLessThanMaxAllowedPort)
	}
}

func TestRandomPortInAllowRange(t *testing.T) {
	// The random port should always be in the range
	randomPort := randomPortInAllowedRange()
	if !isInAllowedRange(randomPort) {
		t.Fatalf("error getting random port in allowed range: %v", randomPort)
	}
}

func TestGetProtocolSpecificHostPortMapUDP(t *testing.T) {
	// Should return a UDP port map
	udpPortMap, err := getProtocolSpecificHostPortMap(TransportProtocolUDP)
	if err != nil {
		t.Fatalf("error getting udp port map: %v", err)
	}
	
	// Confirm return type is protocolSpecificHostPortMap
	udpPortMapType := fmt.Sprintf("%T", udpPortMap)
	expectedPortMapType := "*portbindings.protocolSpecificHostPortMap"
	if udpPortMapType != expectedPortMapType {
		t.Fatalf("error getting protocol specific host port maps. Expected: %v but received: %v", expectedPortMapType, udpPortMapType)
	}
}

func TestGetProtocolSpecificHostPortMapTCP(t *testing.T) {
	// Should return a TCP port map
	tcpPortMap, err := getProtocolSpecificHostPortMap(TransportProtocolTCP)
	if err != nil {
		t.Fatalf("error getting tcp port map: %v", err)
	}
	
	// Confirm return type is protocolSpecificHostPortMap
	tcpPortMapType := fmt.Sprintf("%T", tcpPortMap)
	expectedPortMapType := "*portbindings.protocolSpecificHostPortMap"
	if tcpPortMapType != expectedPortMapType {
		t.Fatalf("error getting protocol specific host port maps. Expected: %v but received: %v", expectedPortMapType, tcpPortMapType)
	}
}

func TestGetProtocolSpecificHostPortMapIncorrectProtocol(t *testing.T) {
	// Should return nil as an incorrect protocol an unsupported was passed
	incorrectProtocol := TransportProtocol("TestTransportIncorrectProtocol")
	hostPortMap, err := getProtocolSpecificHostPortMap(incorrectProtocol)
	if hostPortMap != nil {
		t.Fatalf("error getting incorrect protocol specific host port maps. Should be nil but received: %v", hostPortMap)
	}

	// Confirm that an error is returned
	if err == nil {
		t.Fatalf("error getting incorrect protocol specific host port maps. Expected an err but received nil")
	}
}