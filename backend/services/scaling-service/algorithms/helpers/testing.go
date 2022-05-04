package helpers

import (
	"math/rand"
	"net"
	"time"

	"github.com/google/uuid"
	"github.com/whisthq/whist/backend/services/metadata"
	"github.com/whisthq/whist/backend/services/subscriptions"
)

// SpinUpFakeInstances will create the desired number of instances with random data. Its a helper function
// for local testing, to avoid having to spinup instances on a cloud provider.
func SpinUpFakeInstances(instanceNum int, imageID string, region string) []subscriptions.Instance {
	var fakeInstances []subscriptions.Instance
	for i := 0; i < instanceNum; i++ {
		src := rand.NewSource(time.Now().UnixNano())
		rnd := rand.New(src)
		bytes := make([]byte, 4)
		rnd.Read(bytes)

		fakeInstances = append(fakeInstances, subscriptions.Instance{
			ID:                uuid.NewString(),
			Provider:          "AWS",
			Region:            region,
			ImageID:           imageID,
			ClientSHA:         metadata.GetGitCommit(),
			IPAddress:         net.IPv4(bytes[0], bytes[1], bytes[2], bytes[3]).String(),
			Type:              "g4dn.2xlarge",
			RemainingCapacity: 2,
			Status:            "PRE_CONNECTION",
			CreatedAt:         time.Now(),
			UpdatedAt:         time.Now(),
		})
	}
	return fakeInstances
}
