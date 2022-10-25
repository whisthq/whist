package scaling

import (
	"context"
	"log"
	
	"github.com/whisthq/whist/backend/elegant_monolith/internal"
)

type ScalingService interface {
	internal.Service
	ScaleUpIfNecessary(ctx context.Context, instancesToScale int, imageID string) (instanceIDs []string, err error)
	ScaleDownIfNecessary(ctx context.Context) (instanceIDs []string, err error)
}

func New() (svc ScalingService) {
	return scalingService{
		stopChan: make(chan bool),
	}
}

type scalingService struct {
	// db WhistDBClient
	stopChan chan bool
}

func (scalingSvc scalingService) Start() {
	for {
		log.Printf("Running scaling service...")
		select {
		case <- scalingSvc.stopChan:
			log.Printf("Shutting down scaling service...")
			return
		}
	}
}

func (scalingSvc scalingService) Stop() {
	scalingSvc.stopChan <- true
}


func (scalingService) ScaleUpIfNecessary(ctx context.Context, instancesToScale int, imageID string) (instanceIDs []string, err error) {
	return []string{}, nil
}

func (scalingService) ScaleDownIfNecessary(ctx context.Context) (instanceIDs []string, err error) {
	return []string{}, nil
}
