# Go parameters
GOCMD        = go
GOBUILD      = $(GOCMD) build
GORUN        = $(GOCMD) run
GOCLEAN      = $(GOCMD) clean
GOTEST       = $(GOCMD) test
GOGET        = $(GOCMD) get
GOMODTIDY    = $(GOCMD) mod tidy
GOGEN        = $(GOCMD) generate

# Build parameters
BUILD_FOLDER = build
BINARY_NAME  = ecs-host-service

# Git parameters
GIT_COMMIT := $(shell git rev-list -1 HEAD)
GIT_BRANCH := $(shell git rev-parse --abbrev-ref HEAD)

# Continuous Integration parameters
CI ?= false
DB_URL_DEV ?=
DB_URL_STAGING ?=
DB_URL_PROD ?=

all: build

# Check that the required environment variables for building as part of our
# continuous integration/deployment pipeline on GitHub Actions are set properly.
checkCI:
	$(info building in $(BUILD_FOLDER) with CI set to $(CI)...)
ifeq ($(CI),true)
	# We're building on GitHub Actions, so we need to require non-trivial secrets
	$(info CI is true)
ifeq ($(origin LOGZIO_SHIPPING_TOKEN),file)
	# This means that a replacement secret was not passed in on the command-line or as an environment variable
	$(error "LOGZIO_SHIPPING_TOKEN is not set, but CI is true")
endif
ifeq ($(LOGZIO_SHIPPING_TOKEN),)
	$(error "LOGZIO_SHIPPING_TOKEN is empty, but CI is true")
endif
ifeq ($(origin DB_URL_DEV),file)
	$(error "DB_URL_DEV is not set, but CI is true")
endif
ifeq ($(DB_URL_DEV),)
	$(error "DB_URL_DEV is empty, but CI is true")
endif
ifeq ($(origin DB_URL_STAGING),file)
	$(error "DB_URL_STAGING is not set, but CI is true")
endif
ifeq ($(DB_URL_STAGING),)
	$(error "DB_URL_STAGING is empty, but CI is true")
endif
ifeq ($(origin DB_URL_PROD),file)
	$(error "DB_URL_PROD is not set, but CI is true")
endif
ifeq ($(DB_URL_PROD),)
	$(error "DB_URL_PROD is empty, but CI is true")
endif
else
	$(info CI is false)
endif

build: checkCI
	$(GOMODTIDY)
	$(GOGET) github.com/jschaf/pggen/cmd/pggen
	# Once the ecsagent code is removed, let's generate `./...` instead of just
	# `dbdriver`.
	find $(CURDIR)/dbdriver/queries -type f -name "*.sql.go" | xargs rm
	$(GOGEN) ./dbdriver/...
	$(GOBUILD) -ldflags "\
		-X 'github.com/fractal/fractal/ecs-host-service/metadata.gitCommit=$(GIT_COMMIT)'\
		-X 'github.com/fractal/fractal/ecs-host-service/fractallogger.logzioShippingToken=$(LOGZIO_SHIPPING_TOKEN)' \
		-X 'github.com/fractal/fractal/ecs-host-service/dbdriver.devDatabaseURL=$(DB_URL_DEV)'\
		-X 'github.com/fractal/fractal/ecs-host-service/dbdriver.stagingDatabaseURL=$(DB_URL_STAGING)'\
		-X 'github.com/fractal/fractal/ecs-host-service/dbdriver.prodDatabaseURL=$(DB_URL_PROD)'\
		" -o $(BUILD_FOLDER)/$(BINARY_NAME) ecs-host-service.go

upload: build
	aws s3api put-object --bucket fractal-ecs-host-service --key $(GIT_BRANCH) --body build/ecs-host-service

format:
	(cd ~ && $(GOGET) -u golang.org/x/tools/cmd/goimports)
	goimports -l -w $$(find $(CURDIR) -name "*.go" | grep --invert-match ".sql.go" | xargs)

lint: build
	(cd ~ && $(GOGET) -u golang.org/x/lint/golint)
	go list ./... | grep -v /agent/ | xargs golint -set_exit_status

vet: build
	go list ./... | grep -v /agent/ | xargs go vet

# Note that we should build before we lint or vet, since build updates the fractal dependencies.
.NOTPARALLEL:
check: build vet format lint
	echo "All checks (build, vet, format, and lint) succeeded!"

clean:
	$(GOCLEAN)
	rm -rf $(BUILD_FOLDER)

run: build
	$(info running $(BUILD_FOLDER)/$(BINARY_NAME) with local dev (debug) configuration...)
	# we run the built binary instead of using `go run` because otherwise signal
	# handling for Ctrl+C does not work. We also create the symlink below so that
	# the host service can use the `ubuntu` account's aws credentials to run,
	# even though it runs as root.
ifeq ("$(wildcard /root/.aws/*)","")
	sudo ln -sf /home/ubuntu/.aws /root/.aws
endif
	sudo env "PATH=$(PATH)" APP_ENV=localdev $(BUILD_FOLDER)/$(BINARY_NAME)

runlocaldevwithdb: build
	$(info running $(BUILD_FOLDER)/$(BINARY_NAME) with localdevwithdb configuration...)
ifeq ("$(wildcard /root/.aws/*)","")
	sudo ln -sf /home/ubuntu/.aws /root/.aws
endif
	sudo env "PATH=$(PATH)" APP_ENV=localdevwithdb $(BUILD_FOLDER)/$(BINARY_NAME)

rundev: build
	$(info running $(BUILD_FOLDER)/$(BINARY_NAME) with dev configuration...)
	sudo env "PATH=$(PATH)" APP_ENV=development $(BUILD_FOLDER)/$(BINARY_NAME)

runstaging: build
	$(info running $(BUILD_FOLDER)/$(BINARY_NAME) with staging configuration...)
	sudo env "PATH=$(PATH)" APP_ENV=staging $(BUILD_FOLDER)/$(BINARY_NAME)

runprod: build
	$(info running $(BUILD_FOLDER)/$(BINARY_NAME) with production configuration...)
	sudo env "PATH=$(PATH)" APP_ENV=production $(BUILD_FOLDER)/$(BINARY_NAME)

.PHONY: build clean run rundev runstaging runprod format lint vet check
