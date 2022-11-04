package main

import (
	"fmt"
	"log"
	"sync"

	"github.com/knadh/koanf"
	"github.com/knadh/koanf/parsers/toml"
	"github.com/knadh/koanf/providers/file"

	"github.com/whisthq/whist/backend/elegant_monolith/assign"
	"github.com/whisthq/whist/backend/elegant_monolith/scaling"
)

var config = koanf.New(".")

func main() {
	// Load TOML config
	err := config.Load(file.Provider("config.toml"), toml.Parser())
	if err != nil {
		log.Fatalf("error loading config: %s", err)
	}

	fmt.Println(config.String("cloud.provider"))
	fmt.Println(config.MapKeys("regions"))
	fmt.Println(config.String("regions.us-east-1"))
	fmt.Println(config.String("regions.us-west-1"))
	fmt.Println(config.MapKeys("images"))
	fmt.Println(config.String("images.us-east-1"))
	fmt.Println(config.String("images.us-west-1"))
	fmt.Println(config.Bool("cleaner.enabled"))
	fmt.Println(config.Duration("cleaner.schedule"))

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

	wg.Wait()
	log.Printf("Finished waiting for all services to stop")
}
