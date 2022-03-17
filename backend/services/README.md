# Whist Backend Services

The directory contains the source code for the Whist Backend Services, which are used to power the Whist product by scaling cloud instances up/down based on demand, assigning users to mandelboxes (Whist-enabled Docker containers) for running their cloud browsing, and providing an API for the frontend and third-party backend providers (Auth0 for authentication and Stripe for payments, etc.) to interact with. It is currently split into two major components:

- **The Whist Host-Service**, which is responsible for orchestrating mandelboxes on Whist EC2 instances, which are referred to as **hosts** throughout the codebase. The host-service is responsible for making Docker calls to start and stop mandelboxes, for enabling multiple mandelboxes to run concurrently on the same host by dynamically assigning TTYs, and for passing startup data to the mandelboxes from the users' devices, like DPI, time zone, etc.

- **The Whist Scaling-Service**, which is responsible for responding to load and deploy events to scale instances up and down. The scaling-service will also eventually supersede the [webserver](../webserver/README.md) and handle assigning users to mandelboxes, though that is currently a work-in-progress.

## Development

To consolidate code, we run all backend services as a single Go project, with a `Makefile` in this directory.

The backend services are exclusively written in Go and all additions to this project should be written in Go as well, to keep code clean and maintainable. While the `/host-service` and `/scaling-service` folders are service-specific, all other folders in the `services/` folder are shared between the services.

### Building

Before building the services, make sure you've configured (and logged into) the Heroku and AWS CLIs on your personal development AWS EC2 instance.

To build the services, install Go via `sudo snap install --classic --channel=1.17/stable go`, and run `make build`. Note that we require Go versions >= 1.17, and the that services are only meant to be run on Linux Ubuntu. After installing Go, be sure to add `~/go/bin` to your `$PATH` variable as follows:

```bash
PATH=$PATH:~/go/bin
```

This will build the services under the directory `/build` as `host-service` and `scaling-service`.

### Styling

We use `goimports` and `staticcheck` for Golang linting and coding standards. These are automatically installed when running `make format` or `make lint`. The easiest way to check that your code is ready for review (i.e. is linted, vetted, and formatted) is just to run `make check`. This is what we use to check the code in CI as well.

We don't use `os.exit()` or any of the `log.fatal()` variants, or even plain `panic()`s unless strictly necessary, for use cases with very specific requirements. This is because we want to send out a message to Logz.io (our ELK logging provider) and/or Sentry upon the death of this service (this is done with a `defer` function call, which runs after `logger.Panic()` but not after `exit`). Instead, we exclusively use the variants we defined ourselves in the `logger` package.

We use [contexts](https://golang.org/pkg/context/) from the Go standard library extensively to scope requests and set timeouts. [Brushing up on contexts](https://blog.golang.org/context) might prove useful.

### Getting to know the codebase

The best way to learn about the codebase without worrying about implementation details is to browse the [Backend Services Documentation](https://docs.whist.com/backend/services/). Once you've gotten a high-level overview of the codebase, take a look at the Makefile to understand it, and then start reading through the codebase and writing some code yourself! If something seems confusing to you, feel free to reach out to @djsavvy or @MauAraujo, and once you figure it out, make a PR to comment it in the code!

## Host-Service Specifics

See the `/host-service` folder for the specifics of this service.

## Scaling Service Specifics

See the `/scaling-service` folder for the specifics of this service.
