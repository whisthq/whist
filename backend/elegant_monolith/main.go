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
	server := assign.NewServer(assignSvc)

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
		server.Serve()
	}()

	wg.Add(1)
	go func() {
		defer wg.Done()
		scalingSvc.Start()
	}()

	wg.Wait()
	log.Printf("Finished waiting for all services to stop")
}
