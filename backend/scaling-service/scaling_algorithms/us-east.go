package scaling_algorithms

import (
	"context"
	"sync"

	"github.com/whisthq/whist/core-go/subscriptions"
	"github.com/whisthq/whist/core-go/utils"
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
					// Create context for scaling operation
					scalingCtx, scalingCancel := context.WithCancel(context.Background())

					err := s.VerifyInstanceScaleDown(scalingCtx, instanceEvent, instance)

					// Cancel context once the operation is done
					scalingCancel()

					if err != nil {
						logger.Errorf("Error verifying instance scale down. Error: %v", err)
					}
				}

			case mandelboxEvent := <-s.mandelboxEventChan:
				mandelbox := mandelboxEvent.Data.(subscriptions.Mandelbox)

				if mandelbox.Status == "ALLOCATED" {

				}
			}
		}
	}()

}

func (s *USEastScalingAlgorithm) VerifyInstanceScaleDown(scalingCtx context.Context, event ScalingEvent, instance subscriptions.Instance) error {
	logger.Infof("Verifying instance scale down for event: %v", event)
	// First, verify if the draining instance has mandelboxes running
	mandelboxesRunningQuery := subscriptions.QueryMandelboxesByInstanceName
	queryParams := map[string]interface{}{
		"instance_name": instance.Name,
	}

	err := s.GraphQLClient.Query(scalingCtx, mandelboxesRunningQuery, queryParams)
	if err != nil {
		return utils.MakeError("failed to query database for running mandelboxes with params: %v. Error: %v", queryParams, err)
	}

	// Check underlying struct received...
	logger.Infof("Query is now: %v", mandelboxesRunningQuery)
	//

	// If not, wait until the instance is terminated in AWS
	// waiterClient := new(ec2.DescribeInstancesAPIClient)
	// waiter := ec2.NewInstanceTerminatedWaiter(*waiterClient, func(*ec2.InstanceTerminatedWaiterOptions) {
	// 	logger.Infof("Waiting for instance to terminate on AWS")
	// })

	// waitParams := &ec2.DescribeInstancesInput{
	// 	InstanceIds: []string{instance.Name},
	// }

	// err = waiter.Wait(scalingCtx, waitParams, 5*time.Minute)
	// if err != nil {
	// 	return utils.MakeError("failed waiting for instance %v to terminate from AWS: %v", instance.Name, err)
	// }

	// Once its terminated, verify that it was removed from the database
	mandelboxExistsQuery := subscriptions.QueryMandelboxStatus
	queryParams = map[string]interface{}{
		"mandelbox_id": mandelboxExistsQuery.CloudMandelboxInfo.ID,
	}

	err = s.GraphQLClient.Query(scalingCtx, mandelboxExistsQuery, queryParams)
	if err != nil {
		return utils.MakeError("failed to query database for mandelbox with params: %v. Error: %v", queryParams, err)
	}

	// Check underlying struct received...
	logger.Infof("Query is now: %v", mandelboxExistsQuery)
	//

	// If mandelbox still exists, delete from db. Write mutation to delete mandelbox from db

	return nil
}
