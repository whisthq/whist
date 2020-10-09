# Go parameters
GOCMD        = go
GOBUILD      = $(GOCMD) build
GORUN        = $(GOCMD) run
GOCLEAN      = $(GOCMD) clean
GOTEST       = $(GOCMD) test
GOGET        = $(GOCMD) get

BUILD_FOLDER = build
BINARY_NAME  = ecs-host-service

all: deps build

build:
	$(info building in $(BUILD_FOLDER)...)
	$(GOBUILD) -o $(BUILD_FOLDER)/$(BINARY_NAME) ecs-host-service.go

clean:
	$(GOCLEAN)
	rm -rf $(BUILD_FOLDER)

run: build
	$(info running $(BUILD_FOLDER)/$(BINARY_NAME)...)
# we run the built binary instead of using `go run` because otherwise signal
# handling for Ctrl+C does not work
	sudo env PATH=$PATH $(BUILD_FOLDER)/$(BINARY_NAME)

deps:
	$(GOGET) github.com/docker/docker@v19.03.13

.PHONY: build clean run deps
