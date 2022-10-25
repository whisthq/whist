package main

import (
	"log"
	"sync"

	"github.com/whisthq/whist/backend/elegant_monolith/assign"
	"github.com/whisthq/whist/backend/elegant_monolith/scaling"
)

func main() {
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

	wg.Wait()
	log.Printf("Finished waiting for all services to stop")
}
