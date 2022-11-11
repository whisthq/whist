package assign

import (
	"context"

	"github.com/whisthq/whist/backend/elegant_monolith/internal"
	"github.com/whisthq/whist/backend/elegant_monolith/pkg/dbclient"
	"github.com/whisthq/whist/backend/elegant_monolith/pkg/logger"
)

type AssignService interface {
	internal.Service
	MandelboxAssign(ctx context.Context, regions []string, userEmail string, userID string, commitHash string) (mandelboxID string, ip string, err AssignError)
}

func New() (svc AssignService) {
	return &assignService{
		stopChan: make(chan bool),
		db:       dbclient.NewConnection(),
	}
}

type assignService struct {
	stopChan chan bool
	db       dbclient.DBClient
}

func (assignSvc assignService) Start() {
	for {
		<-assignSvc.stopChan
		logger.Infof("Shutting down assign service...")
		return
	}
}

func (assignSvc assignService) Stop() {
	assignSvc.stopChan <- true
}
