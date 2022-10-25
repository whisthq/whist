package assign

import (
	"context"
	"log"

	"github.com/whisthq/whist/backend/elegant_monolith/internal"
)

type AssignService interface {
	internal.Service
	MandelboxAssign(ctx context.Context, regions []string) (mandelboxID string, ip string, err error)
}

func New() (svc AssignService) {
	return assignService{
		stopChan: make(chan bool),
	}
}

type assignService struct {
	stopChan chan bool
	// db WhistDBClient
}

func (assignSvc assignService) Start() {
	for {
		log.Printf("Running assign service...")
		select {
		case <- assignSvc.stopChan:
			log.Printf("Shutting down assign service...")
			return
		}
	}
}

func (assignSvc assignService) Stop() {
	assignSvc.stopChan <- true
}

func (assignService) MandelboxAssign(ctx context.Context, regions []string) (mandelboxID string, ip string, err error) {
	return "", "", nil
}

