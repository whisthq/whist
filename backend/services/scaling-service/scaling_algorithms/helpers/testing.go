package helpers

import (
	"math/rand"
	"net"
	"time"

	"github.com/google/uuid"
	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/subscriptions"
	"github.com/whisthq/whist/backend/services/types"
)

// SpinUpFakeInstance will create fake instances and associate fake mandelboxes to it.
func SpinUpFakeInstances(instanceNum int, imageID string, region string) ([]subscriptions.Instance, []subscriptions.Mandelbox) {

	var (
		instances   []subscriptions.Instance
		mandelboxes []subscriptions.Mandelbox
	)

	instances = CreateFakeInstances(instanceNum, imageID, region)
	for _, instance := range instances {
		mandelboxes = append(mandelboxes, CreateFakeMandelboxes(instance)...)
	}

	return instances, mandelboxes
}

// CreateFakeInstances will create the desired number of instances with random data. Its a helper function
// for local testing, to avoid having to spinup instances on a cloud provider.
func CreateFakeInstances(instanceNum int, imageID string, region string) []subscriptions.Instance {
	var fakeInstances []subscriptions.Instance
	for i := 0; i < instanceNum; i++ {
		src := rand.NewSource(time.Now().UnixNano())
		rnd := rand.New(src)
		bytes := make([]byte, 4)
		rnd.Read(bytes)
		ip := net.IPv4(bytes[0], bytes[1], bytes[2], bytes[3]).String()

		fakeInstances = append(fakeInstances, subscriptions.Instance{
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

// CreateFakeMandelboxes will create the desired number of mandelboxes with random data. Its a helper function
// for local testing.
func CreateFakeMandelboxes(instance subscriptions.Instance) []subscriptions.Mandelbox {
	var fakeMandelboxes []subscriptions.Mandelbox
	for i := 0; i < int(instance.RemainingCapacity); i++ {
		fakeMandelboxes = append(fakeMandelboxes, subscriptions.Mandelbox{
			ID:         types.MandelboxID(uuid.New()),
			App:        "WHISTIUM",
			InstanceID: instance.ID,
			Status:     "WAITING",
			CreatedAt:  time.Now(),
			UpdatedAt:  time.Now(),
		})
	}

	return fakeMandelboxes
}
