# Go parameters
GOCMD        = go
GOBUILD      = $(GOCMD) build
GOCLEAN      = $(GOCMD) clean
GOTEST       = $(GOCMD) test
GOGET        = $(GOCMD) get

BUILD_FOLDER = build
BINARY_NAME  = ecs_host_service

all: deps build

build:
	$(info building in $(BUILD_FOLDER)...)
	$(GOBUILD) -o $(BUILD_FOLDER)/$(BINARY_NAME) -v
clean:
	$(GOCLEAN)
	rm -rf $(BUILD_FOLDER)

run: build
	$(info running $(BUILD_FOLDER)/$(BINARY_NAME)...)
	@$(BUILD_FOLDER)/$(BINARY_NAME)

deps:
	$(GOGET) github.com/docker/docker@v19.03.13
