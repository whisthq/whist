package scaling_algorithms

import (
	"context"
	"sync"

	"github.com/whisthq/whist/backend/services/scaling-service/dbclient"
	"github.com/whisthq/whist/backend/services/scaling-service/hosts"
	aws "github.com/whisthq/whist/backend/services/scaling-service/hosts/aws"
	"github.com/whisthq/whist/backend/services/subscriptions"
	logger "github.com/whisthq/whist/backend/services/whistlogger"
)

// ScalingAlgorithm is the basic abstraction of the scaling service
// that receives a stream of events and makes calls to the host handler.
type ScalingAlgorithm interface {
	ProcessEvents(context.Context, *sync.WaitGroup)
	CreateEventChans()
	CreateBuffer()
	CreateGraphQLClient(*subscriptions.GraphQLClient)
	CreateDBClient(*dbclient.DBClient)
}

// ScalingEvent is an event that contains all the relevant information
// to make scaling decisions.
type ScalingEvent struct {
	ID     string
	Type   interface{} // The type of event (database, timing, etc.)
	Data   interface{} // Data relevant to the event
	Region string      // Region where the scaling will be performed
}

// DefaultScalingAlgorithm abstracts the shared functionalities to be used
// by all of the different, region-based scaling algorithms.
type DefaultScalingAlgorithm struct {
	Host               hosts.HostHandler
	GraphQLClient      *subscriptions.GraphQLClient
	DBClient           *dbclient.DBClient
	InstanceBuffer     *int32
	Region             string
	InstanceEventChan  chan ScalingEvent
	ImageEventChan     chan ScalingEvent
	ScheduledEventChan chan ScalingEvent
}

// CreateEventChans creates the event channels if they don't alredy exist.
func (s *DefaultScalingAlgorithm) CreateEventChans() {
	// TODO: Only use one chan for database events and
	// one for scheduled events
	if s.InstanceEventChan == nil {
		s.InstanceEventChan = make(chan ScalingEvent, 100)
	}
	if s.ImageEventChan == nil {
		s.ImageEventChan = make(chan ScalingEvent, 100)
	}
	if s.ScheduledEventChan == nil {
		s.ScheduledEventChan = make(chan ScalingEvent, 100)
	}
}

// CreateBuffer initializes the instance buffer.
func (s *DefaultScalingAlgorithm) CreateBuffer() {
	buff := int32(DEFAULT_INSTANCE_BUFFER)
	s.InstanceBuffer = &buff
}

// CreateGraphQLClient sets the graphqlClient to be used by the DBClient for queries and mutations.
func (s *DefaultScalingAlgorithm) CreateGraphQLClient(client *subscriptions.GraphQLClient) {
	if s.GraphQLClient == nil {
		s.GraphQLClient = client
	}
}

// CreateDBClient sets the DBClient to be used when interacting with the database.
func (s *DefaultScalingAlgorithm) CreateDBClient(dbClient *dbclient.DBClient) {
	if s.DBClient == nil {
		s.DBClient = dbClient
	}
}

// ProcessEvents is the main function of the scaling algorithm, it is responsible of processing
// events and executing the appropiate scaling actions. This function is specific for each region
// scaling algorithm to be able to implement different strategies on each region.
func (s *DefaultScalingAlgorithm) ProcessEvents(globalCtx context.Context, goroutineTracker *sync.WaitGroup) {
	if s.Host == nil {
		// TODO when multi-cloud support is introduced, figure out a way to
		// decide which host to use. For now default to AWS.
		handler := &aws.AWSHost{}
		err := handler.Initialize(s.Region)

		if err != nil {
			logger.Errorf("Error starting host on region: %v. Error: %v", err, s.Region)
		}

		s.Host = handler
	}

	// Start algorithm main event loop
	goroutineTracker.Add(1)
	go func() {

		defer goroutineTracker.Done()

		for {
			select {
			case instanceEvent := <-s.InstanceEventChan:
				logger.Infof("Scaling algorithm received an instance database event with value: %v", instanceEvent)
				instance := instanceEvent.Data.(subscriptions.Instance)

				if instance.Status == "DRAINING" {

					goroutineTracker.Add(1)
					go func() {
						defer goroutineTracker.Done()

						// Create context for scaling operation
						scalingCtx, scalingCancel := context.WithCancel(context.Background())
						err := s.VerifyInstanceScaleDown(scalingCtx, instanceEvent, instance)

						// Cancel context once the operation is done
						scalingCancel()

						if err != nil {
							logger.Errorf("Error verifying instance scale down. Error: %v", err)
						}
					}()
				}
			case scheduledEvent := <-s.ScheduledEventChan:
				switch scheduledEvent.Type {
				case "SCHEDULED_SCALE_DOWN_EVENT":
					logger.Infof("Scaling algorithm received a scheduled scale down event with value: %v", scheduledEvent)

					goroutineTracker.Add(1)
					go func() {
						defer goroutineTracker.Done()

						scalingCtx, scalingCancel := context.WithCancel(context.Background())
						err := s.ScaleDownIfNecessary(scalingCtx, scheduledEvent)
						if err != nil {
							logger.Errorf("Error running scale down job on region %v. Err: %v", scheduledEvent.Region, err)
						}

						scalingCancel()
					}()
				case "SCHEDULED_IMAGE_UPGRADE_EVENT":
					logger.Infof("Scaling algorithm received an image upgrade event with value: %v", scheduledEvent)

					goroutineTracker.Add(1)
					go func() {
						defer goroutineTracker.Done()

						logger.Infof("%v", scheduledEvent)

						if scheduledEvent.Data == nil {
							logger.Errorf("Error running image upgrade, event data is nil.")
							return
						}

						scalingCtx, scalingCancel := context.WithCancel(context.Background())

						// Get arguments from scheduled event
						regionImageMap := scheduledEvent.Data.(map[string]interface{})

						err := s.UpgradeImage(scalingCtx, scheduledEvent, regionImageMap[scheduledEvent.Region])
						if err != nil {
							logger.Errorf("Error running image upgrade on region %v. Err: %v", scheduledEvent.Region, err)
						}

						scalingCancel()
					}()
				}
			case <-globalCtx.Done():
				logger.Info("Global context has been cancelled. Exiting from default scaling algorithm event loop...")
				return
			}
		}
	}()
}
