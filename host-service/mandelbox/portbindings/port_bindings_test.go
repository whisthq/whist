package portbindings

import (
	"testing"
)

func TestAllocateZeroPorts(t *testing.T) {
	// Give zero portBinds
	ports, err := Allocate([]PortBinding{})
	
	if err != nil {
		t.Fatalf("error allocating an empty array of port binds. Error: %v", err)
	}

	if numOfPorts := len(ports); numOfPorts != 0 {
		t.Fatalf("error allocating an empty array of port binds. Expected %v ports but got %v", 0, numOfPorts)
	}
}

func TestAllocateAllOrNone(t *testing.T) {
	// Have valid and invalid PortBindings
	testValidPortBinding := PortBinding{
		MandelboxPort: uint16(22),
		Protocol: TransportProtocolTCP,
		HostPort: uint16(0),
		BindIP: "test_ip",
	}

	testValidPortBinding2 := PortBinding{
		MandelboxPort: uint16(22),
		Protocol: TransportProtocolTCP,
		HostPort: uint16(MinAllowedPort),
		BindIP: "test_ip",
	}

	testInvalidPortBinding := PortBinding{
		MandelboxPort: uint16(22),
		Protocol: TransportProtocolTCP,
		HostPort: uint16(MinAllowedPort-1),
		BindIP: "test_ip",
	}

	// Allocate should cause an error as one of the ports are invalid
	ports, err := Allocate([]PortBinding{testValidPortBinding, testValidPortBinding2, testInvalidPortBinding})	

	if err == nil {
		t.Fatalf("error allocating an array of port binds with invalid bindings. Expected err but got nil")
	}

	// Confirm none of the ports are allocated
	if numOfPorts := len(ports); numOfPorts != 0 {
		t.Fatalf("error allocating an array of port binds with invalid bindings. Expected %v ports but got %v", 0, numOfPorts)
	}
}

func TestAllocateAndFree(t *testing.T) {
	// Create two valid port bindings
	testValidPortBinding := PortBinding{
		MandelboxPort: uint16(22),
		Protocol: TransportProtocolTCP,
		HostPort: uint16(0),
		BindIP: "test_ip",
	}

	testValidPortBinding2 := PortBinding{
		MandelboxPort: uint16(22),
		Protocol: TransportProtocolTCP,
		HostPort: uint16(MinAllowedPort),
		BindIP: "test_ip",
	}

	portBindings := []PortBinding{testValidPortBinding, testValidPortBinding2}
	numOfPortBindings := len(portBindings)

	//  Allocate should get 2 valid ports
	ports, err := Allocate(portBindings)
		
	if err != nil {
		t.Fatalf("error allocating an array of port binds. Error: %v", err)
	}

	if numOfPortsReceived := len(ports); numOfPortsReceived != numOfPortBindings {
		t.Fatalf("error allocating an array of port binds. Expected %v ports but got %v", numOfPortBindings, numOfPortsReceived)
	}

	mapToUse, err := getProtocolSpecificHostPortMap(testValidPortBinding.Protocol)
	if err != nil {
		t.Fatalf("error getting host port map.  Error: %v", err)
	}

	portMapsLock.Lock()
	mapSizeBeforeFree := len(*mapToUse)
	portMapsLock.Unlock()

	// Free allocated ports
	Free(ports)

	portMapsLock.Lock()
	defer portMapsLock.Unlock()
	mapSizeAfterFree := len(*mapToUse)

	// Confirm that the number of Free'd ports are as expected
	if mapSizeBeforeFree - numOfPortBindings != mapSizeAfterFree {
		t.Fatalf("error freeing %v port bindings. Expected %v since nothing should be freed but got %v", numOfPortBindings, mapSizeBeforeFree - numOfPortBindings, mapSizeAfterFree)
	}
}

func TestFreeEmptyPortBinds(t *testing.T) {
	mapToUse, err := getProtocolSpecificHostPortMap(TransportProtocolTCP)
	if err != nil {
		t.Fatalf("error getting host port map.  Error: %v", err)
	}

	portMapsLock.Lock()
	mapSizeBeforeFree := len(*mapToUse)
	portMapsLock.Unlock()

	// Free checks if an empty binding is returned
	Free([]PortBinding{})

	portMapsLock.Lock()
	defer portMapsLock.Unlock()
	mapSizeAfterFree := len(*mapToUse)

	// Host port map size should not be affected
	if mapSizeBeforeFree != mapSizeAfterFree {
		t.Fatalf("error freeing empty list of port bindings. Expected %v since nothing should be freed but got %v", mapSizeBeforeFree, mapSizeAfterFree)
	}
}

func TestAllocateSinglePortNotFree(t *testing.T) {
	// Creating a desired binding with host-port being set to 0
	testDesiredBind := PortBinding{
		MandelboxPort: uint16(22),
		Protocol: TransportProtocolTCP,
		HostPort: uint16(MinAllowedPort),
		BindIP: "test_ip",
	}

	portMapsLock.Lock()
	defer portMapsLock.Unlock()

	// Should successfully allocate port
	_, err := allocateSinglePort(testDesiredBind)
	if err != nil {
		t.Fatalf("error allocating a single port with a free hostPort: %v", err)
	}

	// Attempt to claim the same port
	_, err = allocateSinglePort(testDesiredBind)
	if err == nil {
		t.Fatalf("error allocating a single port with an used hostPort. Expected an err but got nil")
	}

	// Free port
	freeSinglePort(testDesiredBind)
}

func TestAllocateSinglePortNotAllowed(t *testing.T) {
	// Creating a desired binding with host-port being set to 0
	testDesiredBind := PortBinding{
		MandelboxPort: uint16(22),
		Protocol: TransportProtocolTCP,
		HostPort: uint16(1),
		BindIP: "test_ip",
	}

	portMapsLock.Lock()
	defer portMapsLock.Unlock()

	// Allocate single port will check if hostPort is within allowed range
	_, err := allocateSinglePort(testDesiredBind)
	if err == nil {
		t.Fatalf("error allocating a single port with a hostPort not in allowed range. Expected an err but got nil")
	}
}

func TestAllocateSinglePortInvalidProtocol(t *testing.T) {
	// Creating a desired binding with an invalid protocol 
	testDesiredBind := PortBinding{
		MandelboxPort: uint16(22),
		Protocol: TransportProtocol("testInvalidProtocol"),
		HostPort: MinAllowedPort,
		BindIP: "test_ip",
	}

	portMapsLock.Lock()
	defer portMapsLock.Unlock()

	// Allocate single port checks if protocol has a valid host port map
	_, err := allocateSinglePort(testDesiredBind)
	if err == nil {
		t.Fatalf("error allocating a single port with invalid protocol. Expected err but got nil")
	}
}

func TestAllocateAndFreeSinglePort(t *testing.T) {
	// Creating a desired binding with a set host-port 
	testDesiredBind := PortBinding{
		MandelboxPort: uint16(22),
		Protocol: TransportProtocolTCP,
		HostPort: MinAllowedPort,
		BindIP: "test_ip",
	}

	portMapsLock.Lock()
	defer portMapsLock.Unlock()

	// Generate new PortBind with same fields
	updatedPortBind, err := allocateSinglePort(testDesiredBind)
	if err != nil {
		t.Fatalf("error allocating a single port with hostPort set to %v. Error: %v", MinAllowedPort, err)
	}

	if updatedPortBind == (PortBinding{}) {
		t.Fatalf("error allocating a single port with hostPort set to %v. Given empty PortBinding", MinAllowedPort)
	}

	// HostPort should have a random port
	if updatedPortBind.HostPort != testDesiredBind.HostPort {
		t.Fatalf("error allocating a single port with a set hostPort. Expected %v but got %v", testDesiredBind.HostPort, updatedPortBind.HostPort)
	}

	// All other fields but HostPort should be the same
	if updatedPortBind.Protocol != testDesiredBind.Protocol || updatedPortBind.MandelboxPort != testDesiredBind.MandelboxPort || updatedPortBind.BindIP != testDesiredBind.BindIP {
		t.Fatalf("error allocating a single port with set hostPort. One or more fields of %v did not match %v", updatedPortBind, testDesiredBind)
	}

	mapToUse, err := getProtocolSpecificHostPortMap(updatedPortBind.Protocol)
	if err != nil {
		t.Fatalf("error getting host port map. Error: %v", err)
	}

	mapSizeBeforeFree := len(*mapToUse)
	freeSinglePort(updatedPortBind)
	mapSizeAfterFree := len(*mapToUse)

	// Only one port should be freed
	if mapSizeBeforeFree - 1 != mapSizeAfterFree {
		t.Fatalf("error freeing single port. Expected map size to be %v but got %v", mapSizeBeforeFree - 1, mapSizeAfterFree)
	}
}

func TestAllocateSinglePortAnyFullHostPortMap(t *testing.T) {
	// Creating a desired binding with host-port being set to 0
	testDesiredBind := PortBinding{
		MandelboxPort: uint16(22),
		Protocol: TransportProtocolTCP,
		HostPort: uint16(0),
		BindIP: "test_ip",
	}

	// Fill out entire map
	portMapsLock.Lock()
	defer portMapsLock.Unlock()
	
	mapToUse, err := getProtocolSpecificHostPortMap(testDesiredBind.Protocol)
	if err != nil {
		t.Fatalf("error getting host port map. Error: %v", err)
	}
	
	for port := MinAllowedPort; port < MaxAllowedPort; port++ {
		(*mapToUse)[uint16(port)] = inUse
	}

	// Generate new PortBind with same fields and new HostPort
	_, err = allocateSinglePort(testDesiredBind)
	if err == nil {
		t.Fatalf("error allocating a single port with hostPortMap full. Expected error but got nil")
	}

	// Clean up entire map
	for port := MinAllowedPort; port < MaxAllowedPort; port++ {
		delete(*mapToUse, uint16(port))
	}

}

func TestAllocateAndFreeSinglePortAny(t *testing.T) {
	// Creating a desired binding with host-port being set to 0
	testDesiredBind := PortBinding{
		MandelboxPort: uint16(22),
		Protocol: TransportProtocolTCP,
		HostPort: uint16(0),
		BindIP: "test_ip",
	}

	portMapsLock.Lock()
	defer portMapsLock.Unlock()

	// Generate new PortBind with same fields and new HostPort
	updatedPortBind, err := allocateSinglePort(testDesiredBind)
	if err != nil {
		t.Fatalf("error allocating a single port with hostPort set to 0. Error: %v", err)
	}

	if updatedPortBind == (PortBinding{}) {
		t.Fatalf("error allocating a single port with hostPort set to 0. Given empty PortBinding")
	}

	// HostPort should have a random port
	if updatedPortBind.HostPort == uint16(0) {
		t.Fatalf("error allocating a single port with hostPort set to 0. Received update PortBinding with hostPort set to 0")
	}

	// The random hostPort should be valid
	if !isInAllowedRange(updatedPortBind.HostPort) {
		t.Fatalf("error checking in allowed range for random host port: %v", updatedPortBind.HostPort)
	}

	// All other fields but HostPort should be the same
	if updatedPortBind.Protocol != testDesiredBind.Protocol || updatedPortBind.MandelboxPort != testDesiredBind.MandelboxPort || updatedPortBind.BindIP != testDesiredBind.BindIP {
		t.Fatalf("error allocating a single port with hostPort set to 0. One or more fields of %v did not match %v", updatedPortBind, testDesiredBind)
	}

	// Check size of table to confirm port have been freed
	mapToUse, err := getProtocolSpecificHostPortMap(updatedPortBind.Protocol)
	if err != nil {
		t.Fatalf("error getting host port map. Error: %v", err)
	}
	
	mapSizeBeforeFree := len(*mapToUse)
	freeSinglePort(updatedPortBind)
	mapSizeAfterFree := len(*mapToUse)

	if mapSizeBeforeFree - 1 != mapSizeAfterFree {
		t.Fatalf("error freeing single port. Expected map size to be %v but got %v", mapSizeBeforeFree - 1, mapSizeAfterFree)
	}
}

func TestFreeSinglePortNonExistent(t *testing.T) {
	testDesiredBind := PortBinding{
		Protocol: TransportProtocolTCP,
		HostPort: MinAllowedPort,
	}

	portMapsLock.Lock()
	defer portMapsLock.Unlock()

	// Freeing a port that has not been assigned
	mapToUse, err := getProtocolSpecificHostPortMap(TransportProtocolTCP)
	if err != nil {
		t.Fatalf("error getting host port map. Error: %v", err)
	}

	mapSizeBeforeFree := len(*mapToUse)
	freeSinglePort(testDesiredBind)
	mapSizeAfterFree := len(*mapToUse)

	if mapSizeBeforeFree != mapSizeAfterFree {
		t.Fatalf("error freeing single non-existent port. Expected map size to be %v but got %v", mapSizeBeforeFree, mapSizeAfterFree)
	}
}

func TestFreeSingleReservedPort(t *testing.T) {
	portMapsLock.Lock()
	defer portMapsLock.Unlock()

	mapToUse, err := getProtocolSpecificHostPortMap(TransportProtocolTCP)
	if err != nil {
		t.Fatalf("error getting host port map. Error: %v", err)
	}

	randomPort := randomPortInAllowedRange()
	testDesiredBind := PortBinding{
		Protocol: TransportProtocolTCP,
		HostPort: randomPort,
	}

	// Set the random port to reserved
	Reserve(testDesiredBind.HostPort, testDesiredBind.Protocol)

	mapSizeBeforeFree := len(*mapToUse)
	freeSinglePort(testDesiredBind)
	mapSizeAfterFree := len(*mapToUse)

	// Reserved port will not be freed
	if mapSizeBeforeFree != mapSizeAfterFree {
		t.Fatalf("error freeing single reserved port. Expected map size to be %v but got %v", mapSizeBeforeFree, mapSizeAfterFree)
	}

	(*mapToUse)[randomPort] = inUse
	freeSinglePort(testDesiredBind)

}

func TestReservePortAvailable(t *testing.T) {
	testDesiredBind := PortBinding{
		Protocol: TransportProtocolTCP,
		HostPort: randomPortInAllowedRange(),
	}

	portMapsLock.Lock()
	defer portMapsLock.Unlock()

	mapToUse, err := getProtocolSpecificHostPortMap(testDesiredBind.Protocol)
	if err != nil {
		t.Fatalf("error getting tcp port map: %v", err)
	}

	// Check if port exists
	if _, exists := (*mapToUse)[testDesiredBind.HostPort]; exists {
		t.Fatalf("error getting free port to reserve")
	}

	Reserve(testDesiredBind.HostPort, testDesiredBind.Protocol)

	// Check if port exists and set to reserved
	if status, exists := (*mapToUse)[testDesiredBind.HostPort]; !exists || status != reserved {
		t.Fatalf("error reserving free port")
	}

	(*mapToUse)[testDesiredBind.HostPort] = inUse
	freeSinglePort(testDesiredBind)
}

func TestReservePortNotAvailable(t *testing.T) {
	testDesiredBind := PortBinding{
		Protocol: TransportProtocolTCP,
		HostPort: randomPortInAllowedRange(),
	}

	portMapsLock.Lock()
	defer portMapsLock.Unlock()

	mapToUse, err := getProtocolSpecificHostPortMap(testDesiredBind.Protocol)
	if err != nil {
		t.Fatalf("error getting tcp port map: %v", err)
	}

	// Add new port to host port map
	(*mapToUse)[testDesiredBind.HostPort] = inUse

	// Port should maintain the original status
	Reserve(testDesiredBind.HostPort, testDesiredBind.Protocol)

	if status, exists := (*mapToUse)[testDesiredBind.HostPort]; !exists || status != inUse {
		t.Fatalf("error reserving port inUse modified host port map. Expected (%v,%v) but got (%v,%v)", true, inUse, exists, status)
	}

	freeSinglePort(testDesiredBind)
}

func TestReservePortInvalidProtocol(t *testing.T) {
	testDesiredBind := PortBinding{
		Protocol: TransportProtocol("testInvalidProtocol"),
		HostPort: randomPortInAllowedRange(),
	}

	portMapsLock.Lock()
	defer portMapsLock.Unlock()

	// Reserve should fail silently as protocol is invalid
	Reserve(testDesiredBind.HostPort, testDesiredBind.Protocol)
}

func TestReservePortOutsideAllowedRange(t *testing.T) {
	testDesiredBind := PortBinding{
		Protocol: TransportProtocol("testInvalidProtocol"),
		HostPort: uint16(MinAllowedPort-1),
	}

	portMapsLock.Lock()
	defer portMapsLock.Unlock()

	mapToUse, err := getProtocolSpecificHostPortMap(TransportProtocolTCP)
	if err != nil {
		t.Fatalf("error getting host port map. Error: %v", err)
	}

	mapSizeBeforeReserve := len(*mapToUse)

	// Reserve should fail silently as port is invalid
	Reserve(testDesiredBind.HostPort, testDesiredBind.Protocol)

	mapSizeAfterReserve := len(*mapToUse)

	// Map will not contain the invalid port
	if mapSizeBeforeReserve != mapSizeAfterReserve {
		t.Fatalf("error reserving port that is not in allowed range. Expected map size to be %v but got %v", mapSizeBeforeReserve, mapSizeAfterReserve)
	}
}

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
	
	// Confirm returudpPortMapn type is protocolSpecificHostPortMap
	if _, ok := interface{}(udpPortMap).(*protocolSpecificHostPortMap); !ok {
		t.Fatalf("error getting protocol specific host port maps")
	}
}

func TestGetProtocolSpecificHostPortMapTCP(t *testing.T) {
	// Should return a TCP port map
	tcpPortMap, err := getProtocolSpecificHostPortMap(TransportProtocolTCP)
	if err != nil {
		t.Fatalf("error getting tcp port map: %v", err)
	}
	
	// Confirm return type is protocolSpecificHostPortMap
	if _, ok := interface{}(tcpPortMap).(*protocolSpecificHostPortMap); !ok {
		t.Fatalf("error getting protocol specific host port maps")
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
