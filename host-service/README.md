# Fractal Host Service

This subfolder contains the code for the Fractal Host Service, which is responsible for orchestrating mandelboxes on Fractal EC2 instances, which are referred to as **hosts** throughout the codebase. The host service is responsible for making Docker calls to start and stop mandelboxes, for enabling multiple mandelboxes to run concurrently on the same host by dynamically assigning TTYs, and for passing startup data to the mandelboxes from the Fractal webserver(s), like DPI. If you are just interested in what endpoints the host service exposes (i.e. for developing on the client application or webserver, check the file `httpserver/server.go`).

## Development

### Building

Before building the service, make sure you've configured (and logged into) the Heroku CLI on your EC2 instance.

To build the service, install Go via `sudo snap install --classic --channel=1.16/stable go`, and then run `make build`. Note that the host service is only meant to build on Go versions >= 1.16 and is only supported on Linux since the introduction of [go-nvml](https://github.com/NVIDIA/go-nvml) as a dependency. Also, be sure to add `~/go/bin` to your `$PATH` variable as follows:

```bash
PATH=$PATH:~/go/bin
```

This will build the service under directory `/build` as `host-service`.

### Running

It is only possible to run the host service on AWS EC2 instances, since the host service code retrieves metadata about the instance on which it is running from the EC2 instance metadata endpoint <http://169.254.169.254/latest/meta-data/>. According to the [EC2 documentation](https://docs.aws.amazon.com/AWSEC2/latest/UserGuide/instancedata-data-retrieval.html), "the IP address `196.254.169.254` is a link-local address and is valid only from the [EC2] instance."

From an EC2 instance, you can run the host service via `make run`. Note that the service must be run as `root` since it manages `systemd` and `docker`, so make sure that your Linux user has permission to use `sudo` and be prepared to supply your password.

If you want to test the host service with our production Sentry configuration, use the command `make runprod`. Note that this will count against our Sentry logging quotas! As such, we only recommend you try to do that on an Fractal-optimized AWS EC2 instance that was started by the webserver (see `host-setup/`).

### Design Decisions

This service will not restart on crash/panic, since that could lead to an inconsistency between the actually running mandelboxes and the data left on the filesystem. Instead, we note that if the service crashes, no new mandelboxes will be able to report themselves to the Fractal webserver(s), meaning there will be no new connections to the EC2 host and once all running mandelboxes are disconnected, the instance will be spun down.

We never use `os.exit()` or any of the `log.fatal()` variants, or even plain `panic()`s, since we want to send out a message to our Fractal webserver(s) and/or Sentry upon the death of this service (this is done with a `defer` function call, which runs after `logger.Panic()` but not after `exit`).

For more details, see the comments and deferred cleanup function at the beginning of `main()`.

We use [contexts](https://golang.org/pkg/context/) from the Go standard library extensively to scope requests and set timeouts. [Brushing up on contexts](https://blog.golang.org/context) might prove useful.

## Styling

We use `goimports` and `golint` for Golang linting and coding standards. These are automatically installed when running `make format` or `make lint`.

The easiest way to check that your code is ready for review (i.e. is linted, vetted, and formatted) is just to run `make check`. This is what we use to check the code in CI as well.

## Publishing

The Fractal host service gets built into our AMIs during deployment.

For testing, you can also use the `upload` target in the makefile, which builds a host service and pushes it to the `fractal-host-service` s3 bucket with value equal to the branch name that `make upload` was run from.

## Getting to know the codebase

The best way to learn about the codebase without worrying about implementation details is to browse the [Host Service Documentation](https://docs.fractal.co/host-service/). Once you've gotten a high-level overview of the codebase, take a look at the Makefile to understand it, and then start reading through the codebase and writing some code yourself! If something seems confusing to you, feel free to reach out to @djsavvy, and once you figure it out, make a PR to comment it in the code!
