package metadata

type (
	InstanceID      string
	InstanceName    string
	ImageID         string
	InstanceType    string
	PlacementRegion string
)

type CloudMetadataRetriever interface {
	GetImageID() (ImageID, error)
	GetInstanceID() (InstanceID, error)
	GetInstanceType() (InstanceType, error)
	GetInstanceName() (InstanceName, error)
	GetPlacementRegion() (PlacementRegion, error)
	GetPublicIpv4() (string, error)
	GetMetadata() (map[string]string, error)
}
