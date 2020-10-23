# Fractal ECS Host Service

This repository contains the Fractal ECS host service, which is installed on AWS EC2 instances running Fractal containers via AWS ECS to perform dynamic tty allocation and ensure new containers can be safely allocated on the host machine as simultaneous requests come in.

## Development

### Building

To build the service, first install Go via your local package manager, i.e. `brew install go`, and then run `make`. Note that this was only tested using Go version >= 1.15.

This will build the service under directory `/build` as `ecs-host-service`.

### Running

You can run locally by running `make run`. Note that the service must be run as root, which the makefile automatically does.

If you want to test the service with our production Sentry configuration, use the command `make runprod`. Note that this will count against our logging quotas!

### Design Decisions

This service will not restart on crash/panic, since that could lead to an inconsistency between the actually running containers and the data left on the filesystem. Instead, we note that if the service crashes no new containers will be able to report themselves to the webserver, so there will be no new connections to the host, and once all running containers are disconnected, the instance will be spun down.

We never use `os.exit()` or any of the `log.fatal()` variants, since we want to send out a message to our webserver and/or Sentry upon the death of this service (this is done with a `defer` function call, which runs after `panic` but not after `exit`).

For more details, see the comments at the beginning of `main()` and `shutdownHostService()`.

## Styling

We use `gofmt` and `golint` for proper linting and coding practices in this project. We use `gofmt` to actually format our Go code, and we use `golint` to enforce proper Go coding practices. We recommend you use both in the pre-commit hooks. You can easily format your code by running `gofmt path-to-file-to-format.go`.

You can also install `golint` on your machine by running `go get -u golang.org/x/lint/golint`. To find out where `golint` was installed, run `go list -f {{.Target}} golang.org/x/lint/golint`. You can then call `golint` via the path returned or by adding it to your `$PATH` without arguments to run it on the whole project. You can easily run Golint via `golint path-to-file-to-lint.go`, which will output a list of recommended behavior improvements to the code.

We also have pre-commit hooks installed for all relevant Go features, including `gofmt` and `golint`, on this project. You can install them by running `pre-commit install`, after having installed the `pre-commit` package via `pip install pre-commit`.

## Publishing

This service gets automatically published with every push to `master` by a GitHub Actions workflow which uploads the executable to an S3 bucket, `fractal-ecs-host-service`, from which a script in the User Data section of the Fractal AMIs pulls the service into the EC2 instances deployed. Pushing to `master` will also trigger an automated Sentry release through GitHub Actions, and our following Sentry logs will be tagged with this release. For more details, see `.github/workflows/publish-build.yml` and `.github/workflows/sentry-release.yml`.
