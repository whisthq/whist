package main

type HostHandler interface {
	SpinUpInstances(region string, numInstances int) (spunUp int, err error)
	SpinDownInstance(instanceId string) error
}
