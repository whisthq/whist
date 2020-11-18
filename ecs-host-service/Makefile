# Go parameters
GOCMD        = go
GOBUILD      = $(GOCMD) build
GORUN        = $(GOCMD) run
GOCLEAN      = $(GOCMD) clean
GOTEST       = $(GOCMD) test
GOGET        = $(GOCMD) get

BUILD_FOLDER = build
BINARY_NAME  = ecs-host-service

GIT_COMMIT := $(shell git rev-list -1 HEAD)

WEBSERVER_AUTH_SECRET_DEV := testwebserverauthsecret
WEBSERVER_AUTH_SECRET_STAGING := testwebserverauthsecret2
WEBSERVER_AUTH_SECRET_PROD := testwebserverauthsecret3

all: deps build

build:
	$(info building in $(BUILD_FOLDER)...)
	$(GOBUILD) -ldflags "\
		-X 'github.com/fractal/ecs-host-service/fractallogger.gitCommit=$(GIT_COMMIT)'\
		-X 'github.com/fractal/ecs-host-service/httpserver.webserverAuthSecretDev=$(WEBSERVER_AUTH_SECRET_DEV)'\
		-X 'github.com/fractal/ecs-host-service/httpserver.webserverAuthSecretStaging=$(WEBSERVER_AUTH_SECRET_STAGING)'\
		-X 'github.com/fractal/ecs-host-service/httpserver.webserverAuthSecretProd=$(WEBSERVER_AUTH_SECRET_PROD)'\
		" -o $(BUILD_FOLDER)/$(BINARY_NAME) ecs-host-service.go

clean:
	$(GOCLEAN)
	rm -rf $(BUILD_FOLDER)

run: build
	$(info running $(BUILD_FOLDER)/$(BINARY_NAME) with debug configuration...)
	# we run the built binary instead of using `go run` because otherwise signal
	# handling for Ctrl+C does not work
	sudo env PATH=$PATH APP_ENV=development $(BUILD_FOLDER)/$(BINARY_NAME)

runprod: build
	$(info running $(BUILD_FOLDER)/$(BINARY_NAME) with production configuration...)
	sudo env PATH=$PATH USE_PROD_SENTRY=1 APP_ENV=production $(BUILD_FOLDER)/$(BINARY_NAME)

deps:
	$(GOGET) github.com/docker/docker@v19.03.13

.PHONY: build clean run runprod deps
