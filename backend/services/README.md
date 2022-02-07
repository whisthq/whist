# Whist Backend Services

This subfolder contains the code for the Whist Backend Services:

- **The Whist Host Service**, which is responsible for orchestrating mandelboxes on Whist EC2 instances, which are referred to as **hosts** throughout the codebase. The host service is responsible for making Docker calls to start and stop mandelboxes, for enabling multiple mandelboxes to run concurrently on the same host by dynamically assigning TTYs, and for passing startup data to the mandelboxes from the Whist webserver(s), like DPI. If you are just interested in what endpoints the host service exposes (i.e. for developing on the client application or webserver, check the file `httpserver/server.go`).
- **The Whist Scaling Service**, which is responsible for responding to load and deploy events to scale instances up and down. The scaling service will also eventually supersede the [webserver](../webserver/README.md) and handle assigning users to mandelboxes, though that is currently a work-in-progress.

## Development

### Building

Before building the service, make sure you've configured (and logged into) the Heroku CLI on your EC2 instance.

To build the services, install Go via `sudo snap install --classic --channel=1.17/stable go`, and then run `make build`. Note that we require Go versions >= 1.17, and the host service can currently only run on Linux. After installing Go, be sure to add `~/go/bin` to your `$PATH` variable as follows:

```bash
PATH=$PATH:~/go/bin
```

This will build the services under directory `/build` as `host-service` and `scaling-service`.

### Styling

We use `goimports` and `staticcheck` for Golang linting and coding standards. These are automatically installed when running `make format` or `make lint`.

The easiest way to check that your code is ready for review (i.e. is linted, vetted, and formatted) is just to run `make check`. This is what we use to check the code in CI as well.

We don't use `os.exit()` or any of the `log.fatal()` variants, or even plain `panic()`s unless strictly necessary, for use cases with very specific requirements. This is because we want to send out a message to our Whist webserver(s) and/or Sentry upon the death of this service (this is done with a `defer` function call, which runs after `logger.Panic()` but not after `exit`). Instead, we exclusively use the variants we defined ourselves in the `logger` package.

We use [contexts](https://golang.org/pkg/context/) from the Go standard library extensively to scope requests and set timeouts. [Brushing up on contexts](https://blog.golang.org/context) might prove useful.

### Getting to know the codebase

The best way to learn about the codebase without worrying about implementation details is to browse the [Backend Services Documentation](https://docs.whist.com/backend/services/). Once you've gotten a high-level overview of the codebase, take a look at the Makefile to understand it, and then start reading through the codebase and writing some code yourself! If something seems confusing to you, feel free to reach out to @djsavvy or @MauAraujo, and once you figure it out, make a PR to comment it in the code!

## Host Service Specifics

### Running the Host Service

It is only possible to run the host service on AWS EC2 instances, since the host service code retrieves metadata about the instance on which it is running from the EC2 instance metadata endpoint <http://169.254.169.254/latest/meta-data/>. According to the [EC2 documentation](https://docs.aws.amazon.com/AWSEC2/latest/UserGuide/instancedata-data-retrieval.html), "the IP address `196.254.169.254` is a link-local address and is valid only from the [EC2] instance."

From an EC2 instance, you can run the host service via `make run_host_service`. Note that the service must be run as `root` since it manages `systemd` and `docker`, so make sure that your Linux user has permission to use `sudo` and be prepared to supply your password, if necessary on your system.

Note that you can test the host service with different environment configurations by using the `make_run_host_service_*` Makefile target variations.

### Running Chrome with a development instance

There are two ways of running development mandelboxes on the host service. The first is to start the mandelboxes directly on your development instance, and the second is to connect a local client app on your machine to the host service on your development instance. The instructions below assume you have already gone through the [host setup readme](https://github.com/whisthq/whist/blob/dev/host-setup/README.md#setting-up-a-development-instance) and built the `browsers/chrome` image on your dev instance.

#### Connecting with the protocol client

1. Start your instance, ssh into it and go to `~/whist/backend/services`, then run `make run_host_service`.
2. Once you see the host enter the event loop, open another terminal window, ssh into your instance, go to `~/whist/mandelboxes` and run `./run_local_mandelbox_image.sh browsers/chrome`. This should start a mandelbox, and you should see the output give a command to run the protocol client on your machine.
3. Run the protocol client on your local machine (follow the [protocol readme](../protocol/README.md#building-the-protocol)).
4. You should see a Chrome window open on your machine.

#### Connecting the client application to your development instance

1. Start your instance, ssh into it and go to `~/whist/backend/services`, then run `make run_host_service` and wait for the event loop to start. Take note of your dev instance ip address.
2. On your local machine, go to `frontend/client-applications` and run the command
   `TESTING_LOCALDEV_HOST_IP=<your instance ip> yarn test:manual localdevHost`. You can also export the `TESTING_LOCALDEV_HOST_IP` env var first and then run `yarn test:manual localdevHost`.
3. The client should open up a Chrome window.

### Working with the JSON transport endpoint

The host service communicates with the client through the `json_transport` endpoint. The purpose of this endpoint is to send data directly from client to host service, which in turn writes a `config.json` file for the protocol to read. You can find the spec of this project on [this notion doc](https://www.notion.so/whisthq/4d91593ea0e0438b8bdb14c25c219d55?v=0c3983cf062d4c3d96ac2a65eb31761b&p=21ada58db10249c2bce9158578873261). If you need to add a feature or test the json transport, you can follow the steps above. You can modify the values sent from the client [here](https://github.com/whisthq/whist/blob/9cd531e5ea52a7c5abe4c1e21c8ce6e83e21170f/client-applications/src/main/flows/mandelbox/index.ts#L30). If you work on the protocol side, just edit the values sent on the client and then look for the `/whist/<mandelboxID>/resourceMappings/config.json` file and the data you sent should be there.

### Setting up Hasura and a local database

The host service uses a pubsub architecture to fire the mandelbox spinup and to shutdown when its marked as `DRAINING` on the database. You can see the analysis behind this decision in [this document](https://www.notion.so/whisthq/Implementing-a-PubSub-822ddcbcdde545e89379e7c7dfa25d71). **This setup is only necessary if you need to test the database/pubsub interaction**. Follow the steps below, or follow this [guide](https://hasura.io/docs/latest/graphql/core/getting-started/docker-simple.html).

1. Download the `docker-compose.yml` file with:
   `curl https://raw.githubusercontent.com/hasura/graphql-engine/stable/install-manifests/docker-compose/docker-compose.yaml -o docker-compose.yml`.
2. Add a `ports` entry to the database service inside the `docker-compose.yml` file. This exposes the database so it can be accessed. For example: `ports: 5432:5432`
3. Run the containers with `docker-compose up -d`.
4. Open up the Hasura console on `http://localhost:8080/console` and add the postgres database (select the Database URL option) with the url `postgres://postgres:postgrespassword@postgres:5432/postgres`.
5. Hasura should add the database and show the existing schemas on the console. Now you can dump the development database schema using the command provided in the [DB Migration README](https://github.com/whisthq/whist/blob/dev/.github/actions/db-migration/README.md#command-to-dump-the-database-schema).

Once you have the schema on your local database and Hasura running, it's ready to test!

### Running and testing the pubsub/database interactions

**The steps below are only needed if you are testing the pubsub interaction on the host service**. Once you have Hasura and postgres on your development instance, the next thing to do is add your instance to the database. You can insert with this command:

```SQL
INSERT INTO whist.instances VALUES ('<your instance id>', 'AWS', 'us-east-1', '<instance image id>', '<commit hash you are on>', '54.89.132.206', 'g4dn.xlarge', 2, 'PRE_CONNECTION', '2022-02-01T20:46:47.500Z', '2022-02-01T20:46:47.500Z');
```

Make sure to change `<your instance id>` on the command to your actual instance id, `<the commit hash you are on>` to the actual commit hash, `<instance image id>` to the image id your instance is running in, and verify the status is `PRE_CONNECTION`.

Once you have added the row to the database, start the host service with `make run_host_service_localdevwithdb` and verify it starts the Hasura subscriptions and enters the event loop correctly.

After this, if you want to test the `SpinUpMandelbox` function by "allocating" a mandelbox for your instance on the local database. Do this by adding a row to the `mandelbox_info` table with this command:

```SQL
`INSERT INTO whist.mandelboxes VALUES ('<a valid UUID>', 'chrome', '<Your instance id>', 'test-user-id', 1234567890, 'ALLOCATED', '2022-02-01T20:46:47.500Z');`
```

Again, verify that `<your instance name>` is your actual instance name, and that the status is `ALLOCATED`.

When you "allocate" a mandelbox for your instance on your database, the pubsub should immediately detect the change and fire the `SpinUpMandelbox` function, and then you can test whatever you need.

To test the `DrainAndShutdown` function, which is also tied to the pubsub, change the status of your instance to `DRAINING` on your local database, this should immediately fire the shutdown and cancel the context.

#### Testing the pubsub with development database

If, for some reason, you need to test the pubsub with the dev Hasura server, you don't need to do the local setup. Just run follow the same steps for adding your instance and allocating a mandelbox on the development database (change the test values to something real).

Before starting the host service export your heroku api token `export HEROKU_API_TOKEN=<your api token>` and the logzio token `export LOGZIO_SHIPPING_TOKEN=<logzio shipping token>`, then you can start the host service with `make run_host_service_dev`.

### Design Decisions on the Host Service

This service will not restart on crash/panic, since that could lead to an inconsistency between the actually running mandelboxes and the data left on the filesystem. Instead, we note that if the service crashes, no new mandelboxes will be able to report themselves to the Whist webserver(s), meaning there will be no new connections to the EC2 host and once all running mandelboxes are disconnected, the instance will be spun down.

For more details on this and the whole host-service lifecycle, see the comments and deferred cleanup function at the beginning of `main()`.

### Publishing the Host Service

The Whist host service gets built into our AMIs during deployment.

## Scaling Service Specifics

### Structure

The scaling service is an event driven system, which is composed of three main parts: the event handler, the scaling algorithm, and the host handler. It follows the same design conventions as the host service, but with a more event-focused approach. As a result, it uses go channels, goroutines and contexts heavily. It is recommended to be familiar with these concepts while working on the scaling service. A more detailed write up of the scaling service is available [here](https://www.notion.so/whisthq/4d91593ea0e0438b8bdb14c25c219d55?v=0c3983cf062d4c3d96ac2a65eb31761b&p=1a8d9b668a8442e79778fb41df01c3e8).

#### Event Handler

The event handler can be seen as the main process in the scaling service. It is responsible of creating and initializing database subscriptions, scheduled events, and scaling algorithms, which are indexed on a sync map. It then receives events, and sends them to the appropiate channels. In a sense, this is the simplest part of the scaling service.

#### Scaling Algorithm

Scaling algorithms are abstracted on the `ScalingAlgorithm` interface, which has generic methods that every scaling algorithm will need. The intention behind making a simple interface is that each scaling algorithm can be extended as necessary to perform different scaling strategies, so that they can be swaped by the event handler with ease. As a start, the default scaling algorithm was created, which contains logic similar to the one in the webserver which can be applied to any region.

In overall, a scaling algorithm has the following structure:

- The `actions` file, which contains code for the scaling actions (scaling up/down). This file is the most important one as the actions make the scaling decisons.
- The `config` file contains constants and variables that are used by the scaling actions, such as the size of the buffer and bundled regions.
- The main file (in the default scaling algorithm its named `default`). Here are the method implementations to satisfy the `ScalingAlgorithm` interface. It also has the logic for starting actions based on the received events.

#### DB Client

This package is the equivalent of the `dbdriver` package from the host service. It interacts directly with the database by using a Hasura client. The intention behind doing a separate package is to abstract all the interactions with the database, such that the scaling algorithm simply calls a function with the necessary parameters and gets the result from the database, without interacting directly with it. The package also contains types necessary for parsing requests/responses from Hasura.

#### Host Handler

Host handlers are abstracted on the `HostHandler` interface, which has the basic methods necessary to perform scaling actions. Each host handler implementation is meant to deal directly with the cloud provider's sdk. By abstracting cloud provider specific logic behind an interface, the scaling actions can be made in an agnostic way, so that multi-cloud support can be added easily. The structure of a host handler is very straightforward , it only has a `config` file with configuration variables specific to the cloud provider, and the main host file where the interface methods are implemented.

## Running the scaling algorithm

The scaling service can be run locally on your computer with the command `make run_scaling_service`. Make sure to have your AWS credentials configured with `aws configure` so that the scaling service is able to start/stop instances. Another thing to keep in mind is that for local testing you need to provide a database (follow the steps above) and add valid instance images to it.

## Implemented scaling algorithms

The following scaling algorithms are fully implemented on the scaling service:

1. **Default scaling algorithm**: this is a general solution that works well on any region, and includes all of the functionalities the team has used in the past. It uses an instance buffer to maintain active instances on each region, includes retry logic when launching instances, and atomic image upgrades. It also verifies the correct termination of instances and notifies of any errors.

## Publishing the scaling service

The Whist host service gets deployed to Heroku during deployment. The logic for deploying can be found in the `deploy.sh` script, which is responsible of generating necessary files and pushing the new changes to Heroku.
