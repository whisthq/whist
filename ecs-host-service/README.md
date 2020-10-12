# Fractal ECS Host Service

This repository contains the Fractal ECS host service, which is installed on AWS EC2 instances running Fractal containers via AWS ECS to perform dynamic tty allocation and ensure new containers can be safely allocated on the host machine as simultaneous requests come in.

## Development

### Building

To build the service, first install Go via your local package manager, i.e. `brew install go`, and then run `make`. Note that this was only tested using Go version >= 1.15.

This will build the service under directory `/build` as `ecs-host-service`.

### Running

You can run by running `make run`. Note that the service must be run as root, which the makefile automatically does.

### Design Decisions

This service will not restart on crash/panic, since that could lead to an inconsistency between the actually running containers and the data left on the filesystem. Instead, we note that if the service crashes no new containers will be able to report themselves to the webserver, so there will be no new connections to the host, and once all running containers are disconnected, the instance will be spun down.

We never use `os.exit()` or any of the `log.fatal()` variants, since we want to send out a message to our webserver and/or Sentry upon the death of this service (this is done with a `defer` function call, which runs after `panic` but not after `exit`).

For more details, see the comments at the beginning of `main()` and `shutdownHostService()`.

## Styling






## Publishing

This service gets automatically published with every push to `master` by a GitHub Actions workflow which uploads the executable to an S3 bucket, `fractal-ecs-host-service`, from which a script in the User Data section of the Fractal AMIs pulls the service into the EC2 instances deployed. For more details, see `.github/workflows/publish-build.yml`.
