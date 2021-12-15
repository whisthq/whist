package scaling_algorithms

import (
	"context"
	"sync"

	"github.com/whisthq/whist/core-go/subscriptions"
	logger "github.com/whisthq/whist/core-go/whistlogger"
	"github.com/whisthq/whist/scaling-service/hosts"
)

type USEastScalingAlgorithm struct {
	*BaseScalingAlgorithm
}

// ProcessEvents is the main function of the scaling algorithm, it is responsible of processing
// events and executing the appropiate scaling actions. This function is specific for each region
// scaling algorithm to be able to implement different strategies on each region.
func (s USEastScalingAlgorithm) ProcessEvents(goroutineTracker *sync.WaitGroup) {
	if s.Host == nil {
		// TODO when multi-cloud support is introduced, figure out a way to
		// decide which host to use. For now default to AWS.
		handler := &hosts.AWSHost{}
		err := handler.Initialize(s.Region)

		if err != nil {
			logger.Errorf("Error starting host on USEast. Error: %v", err)
		}

		s.Host = handler
	}

	goroutineTracker.Add(1)
	go func() {

		for {
			logger.Infof("Scaling algorithm listening for events...")

			select {
			case instanceEvent := <-s.instanceEventChan:
				instance := instanceEvent.Data.(subscriptions.Instance)

				if instance.Status == "DRAINING" {
					s.VerifyInstanceScaleDown(instanceEvent, instance)
				}

			case mandelboxEvent := <-s.mandelboxEventChan:
				mandelbox := mandelboxEvent.Data.(subscriptions.Mandelbox)

				if mandelbox.Status == "ALLOCATED" {

				}
			}
		}
	}()

}

func (s *USEastScalingAlgorithm) VerifyInstanceScaleDown(event ScalingEvent, instance subscriptions.Instance) {
	scalingCtx, scalingCancel := context.WithCancel(context.Background())
	// First, verify if the draining instance has mandelboxes running
	query := subscriptions.QueryMandelboxesByInstanceName
	queryParams := map[string]interface{}{
		"instance_name": instance.InstanceName,
	}

	s.GraphQLClient.Query(scalingCtx, query, queryParams)
	// If not, poll AWS until the instance terminates

	// Once its terminated, verify that it was removed from the database
	// query = subscriptions.QueryMandelboxStatus
	// queryParams = map[string]interface{}{
	// 	"mandelbox_id": query.CloudMandelboxInfo.MandelboxID,
	// }

	// s.GraphQLClient.Query(scalingCtx, query, queryParams)

	// // If mandelbox still exists, delete from db

	// Cancel context once operation is done
	scalingCancel()
}
