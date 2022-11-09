package main

import (
	"context"
	"log"
	"sync"

	"github.com/whisthq/whist/backend/elegant_monolith/assign"
	"github.com/whisthq/whist/backend/elegant_monolith/internal/config"
	"github.com/whisthq/whist/backend/elegant_monolith/scaling"
)

func main() {
	globalCtx, globalCancel := context.WithCancel(context.Background())
	defer globalCancel()

	// Load config from files and command line
	err := config.Initialize(globalCtx)
	if err != nil {
		// Exit cleanly
		return
	}

	// Load config from db ??

	// Start services
	assignSvc := assign.New()
	assignSrv := assign.NewServer(assignSvc)

	scalingSvc := scaling.New()

	wg := &sync.WaitGroup{}

	wg.Add(1)
	go func() {
		defer wg.Done()
		assignSvc.Start()
	}()

	wg.Add(1)
	go func() {
		defer wg.Done()
		assignSrv.Serve()
	}()

	wg.Add(1)
	go func() {
		defer wg.Done()
		scalingSvc.Start()
	}()

	_, _, err = assignSvc.MandelboxAssign(context.Background(), []string{}, "", "", "")
	if err != nil {
		log.Printf("err in assign: %s", err)
	}

	wg.Wait()
	log.Printf("Finished waiting for all services to stop")
}
