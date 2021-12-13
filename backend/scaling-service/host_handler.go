package hosts

type HostHandler interface {
	Initialize(region string) error
	SpinUpInstances(numInstances *int32, imageID string) (spunUp int, err error)
	SpinDownInstances(instanceIDs []string) error
}
