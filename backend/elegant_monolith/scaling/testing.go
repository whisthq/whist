package scaling

import (
	"math/rand"
	"net"
	"time"

	"github.com/google/uuid"
	"github.com/whisthq/whist/backend/elegant_monolith/internal"
)

// spinUpFakeInstance will create fake instances and associate fake mandelboxes to it.
func spinUpFakeInstances(instanceNum int, imageID string, region string) ([]internal.Instance, []internal.Mandelbox) {

	var (
		instances   []internal.Instance
		mandelboxes []internal.Mandelbox
	)

	instances = createFakeInstances(instanceNum, imageID, region)
	for _, instance := range instances {
		mandelboxes = append(mandelboxes, createFakeMandelboxes(instance)...)
	}

	return instances, mandelboxes
}

// createFakeInstances will create the desired number of instances with random data. Its a helper function
// for local testing, to avoid having to spinup instances on a cloud provider.
func createFakeInstances(instanceNum int, imageID string, region string) []internal.Instance {
	var fakeInstances []internal.Instance
	for i := 0; i < instanceNum; i++ {
		src := rand.NewSource(time.Now().UnixNano())
		rnd := rand.New(src)
		bytes := make([]byte, 4)
		rnd.Read(bytes)
		ip := net.IPv4(bytes[0], bytes[1], bytes[2], bytes[3]).String()

		fakeInstances = append(fakeInstances, internal.Instance{
			ID:                uuid.NewString(),
			Provider:          "AWS",
			Region:            region,
			ImageID:           imageID,
			ClientSHA:         metadata.GetGitCommit(),
			IPAddress:         ip + "/24",
			Type:              "g4dn.2xlarge",
			RemainingCapacity: 2,
			Status:            "ACTIVE",
			CreatedAt:         time.Now(),
			UpdatedAt:         time.Now(),
		})
	}
	return fakeInstances
}

// createFakeMandelboxes will create the desired number of mandelboxes with random data. Its a helper function
// for local testing.
func createFakeMandelboxes(instance internal.Instance) []internal.Mandelbox {
	var fakeMandelboxes []internal.Mandelbox
	for i := 0; i < int(instance.RemainingCapacity); i++ {
		fakeMandelboxes = append(fakeMandelboxes, internal.Mandelbox{
			ID:         uuid.NewString(),
			App:        "WHISTIUM",
			InstanceID: instance.ID,
			Status:     "WAITING",
			CreatedAt:  time.Now(),
			UpdatedAt:  time.Now(),
		})
	}

	return fakeMandelboxes
}
