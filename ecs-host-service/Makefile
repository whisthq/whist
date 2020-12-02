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

CI ?= false
WEBSERVER_AUTH_SECRET_DEV ?= testwebserverauthsecretdev
WEBSERVER_AUTH_SECRET_STAGING ?= testwebserverauthsecretstaging
WEBSERVER_AUTH_SECRET_PROD ?= testwebserverauthsecretprod


all: deps build

checkCI:
	$(info building in $(BUILD_FOLDER) with CI set to $(CI)...)
ifeq ($(CI),true)
# We're building on GHA, so we need to require non-trivial secrets
	$(info CI is true)
ifeq ($(origin WEBSERVER_AUTH_SECRET_DEV),file)
# This means that a replacement secret was not passed in on the command-line or as an environment variable
	$(error "WEBSERVER_AUTH_SECRET_DEV is not set, but CI is true")
endif
ifeq ($(WEBSERVER_AUTH_SECRET_DEV),)
	$(error "WEBSERVER_AUTH_SECRET_DEV is empty, but CI is true")
endif
ifeq ($(origin WEBSERVER_AUTH_SECRET_STAGING),file)
	$(error "WEBSERVER_AUTH_SECRET_STAGING is not set, but CI is true")
endif
ifeq ($(WEBSERVER_AUTH_SECRET_STAGING),)
	$(error "WEBSERVER_AUTH_SECRET_STAGING is empty, but CI is true")
endif
ifeq ($(origin WEBSERVER_AUTH_SECRET_PROD),file)
	$(error "WEBSERVER_AUTH_SECRET_PROD is not set, but CI is true")
endif
ifeq ($(WEBSERVER_AUTH_SECRET_PROD),)
	$(error "WEBSERVER_AUTH_SECRET_PROD is empty, but CI is true")
endif
else
	$(info CI is false)
endif

build: checkCI
	$(GOBUILD) -ldflags "\
		-X 'github.com/fractal/fractal/ecs-host-service/fractallogger.gitCommit=$(GIT_COMMIT)'\
		-X 'github.com/fractal/fractal/ecs-host-service/httpserver.webserverAuthSecretDev=$(WEBSERVER_AUTH_SECRET_DEV)'\
		-X 'github.com/fractal/fractal/ecs-host-service/httpserver.webserverAuthSecretStaging=$(WEBSERVER_AUTH_SECRET_STAGING)'\
		-X 'github.com/fractal/fractal/ecs-host-service/httpserver.webserverAuthSecretProd=$(WEBSERVER_AUTH_SECRET_PROD)'\
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
