package hosts

type HostHandler interface {
	Initialize() error
	SpinUpInstances(region string, numInstances int) (spunUp int, err error)
	SpinDownInstance(instanceId string) error
}
