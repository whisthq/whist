package scaling

import (
	"context"
	"log"
	"time"

	"github.com/whisthq/whist/backend/elegant_monolith/internal"
	"github.com/whisthq/whist/backend/elegant_monolith/internal/config"
	"github.com/whisthq/whist/backend/elegant_monolith/pkg/dbclient"
	"github.com/whisthq/whist/backend/elegant_monolith/pkg/logger"
)

type ScalingService interface {
	internal.Service
	ScaleUpIfNecessary(ctx context.Context, instancesToScale int, imageID string, region string) (instanceIDs []string, err error)
	ScaleDownIfNecessary(ctx context.Context, region string) (instanceIDs []string, err error)
}

func New() (svc ScalingService) {
	return scalingService{
		stopChan: make(chan bool),
		db:       dbclient.NewConnection(),
	}
}

type scalingService struct {
	host
	stopChan chan bool
	db       dbclient.DBClient
}

func (scalingSvc scalingService) Start() {
	ticker := time.NewTicker(config.GetScalingInterval())
	for {
		logger.Infof("Running scaling service...")
		select {
		case <-scalingSvc.stopChan:
			log.Printf("Shutting down scaling service...")
			return
		case <-ticker.C:
		}
	}
}

func (scalingSvc scalingService) Stop() {
	scalingSvc.stopChan <- true
}
