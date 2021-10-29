# Whist Host Service

This subfolder contains the code for the Whist Host Service, which is responsible for orchestrating mandelboxes on Whist EC2 instances, which are referred to as **hosts** throughout the codebase. The host service is responsible for making Docker calls to start and stop mandelboxes, for enabling multiple mandelboxes to run concurrently on the same host by dynamically assigning TTYs, and for passing startup data to the mandelboxes from the Whist webserver(s), like DPI. If you are just interested in what endpoints the host service exposes (i.e. for developing on the client application or webserver, check the file `httpserver/server.go`).

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

If you want to test the host service with our production Sentry configuration, use the command `make runprod`. Note that this will count against our Sentry logging quotas! As such, we only recommend you try to do that on an Whist-optimized AWS EC2 instance that was started by the webserver (see `host-setup/`).

### Running Chrome with a development instance

There are two ways of running development mandelboxes on the host service. The first is to start the mandelboxes directly on your development instance, and the second is to connect a local client app on your machine to the host service on your development instance. The instructions below assume you have already gone through the [host setup readme](https://github.com/fractal/fractal/blob/dev/host-setup/README.md#setting-up-a-development-instance) and built the `browsers/chrome` image on your dev instance.

#### Connecting with the protocol client

1. Start your instance, ssh into it and go to `~/fractal/host-service`, then run `make run`.
2. Once you see the host enter the event loop, open another terminal window, ssh into your instance, go to `~/fractal/host-service/mandelboxes` and run `./run_local_mandelbox_image.sh browsers/chrome`. This should start a mandelbox and you should see the output give a command to run the protocol client on your machine.
3. Run the protocol client on your local machine (follow the [protocol readme](https://github.com/fractal/fractal/tree/dev/protocol#building-the-protocol)).
4. You should see a Chrome window open on your machine.

#### Connecting the client application to your development instance

1. Start your instance, ssh into it and go to `~/fractal/host-service`, then run `make run` and wait for the event loop to start. Take note of your dev instance ip address.
2. On your local machine, go to `fractal/client-applications` and run the command
   `TESTING_LOCALDEV_HOST_IP=<your instance ip> yarn test:manual localdevHost`. You can also export the `TESTING_LOCALDEV_HOST_IP` env var first and then run `yarn test:manual localdevHost`.
3. The client should open up a Chrome window.

### Working with the JSON transport endpoint

The host service communicates with the client through the `json_transport` endpoint. The purpose of this endpoint is to send data directly from client to host service, which in turn writes a `config.json` file for the protocol to read. You can find the spec of this project on [this notion doc](https://www.notion.so/tryfractal/4d91593ea0e0438b8bdb14c25c219d55?v=0c3983cf062d4c3d96ac2a65eb31761b&p=21ada58db10249c2bce9158578873261). If you need to add a feature or test the json transport, you can follow the steps above. You can modify the values sent from the client [here](https://github.com/fractal/fractal/blob/9cd531e5ea52a7c5abe4c1e21c8ce6e83e21170f/client-applications/src/main/flows/mandelbox/index.ts#L30). If you work on the protocol side, just edit the values sent on the client and then look for the `/fractal/<mandelboxID>/resourceMappings/config.json` file and the data you sent should be there.

### Set up Hasura and a local database

The host service uses a pubsub architecture to fire the mandelbox spinup and to shutdown when its marked as `DRAINING` on the database. You can see the analysis behind this decision in [this document](https://www.notion.so/tryfractal/Implementing-a-PubSub-822ddcbcdde545e89379e7c7dfa25d71). **This setup is only necessary if you need to test the database/pubsub interaction**. Follow the steps below, or follow this [guide](https://hasura.io/docs/latest/graphql/core/getting-started/docker-simple.html).

1. Download the `docker-compose.yml` file with:
   `curl https://raw.githubusercontent.com/hasura/graphql-engine/stable/install-manifests/docker-compose/docker-compose.yaml -o docker-compose.yml`
2. Run the containers with `docker-compose up -d`
3. Open up the Hasura console on `http://localhost:8080/console` and add the postgres database (select the Database URL option) with the url `psql postgres://postgres:postgrespassword@127.0.0.1:5432/postgres`
4. Hasura should add the database and show the existing schemas on the console. Now you can dump the development database schema using [this command](https://github.com/fractal/fractal/tree/dev/.github/actions/db-migration#command-to-dump-the-database-schema)

Once you have the schema on your local database and Hasura running, it's ready to test!

### Running and testing the pubsub/database interactions

**The steps below are only needed if you are testing the pubsub interaction on the host service**. Once you have Hasura and postgres on your development instance, the next thing to do is add your instance to the database. You can insert with this command:

```
INSERT INTO "cloud"."instance_info" ("ip","location","aws_ami_id","aws_instance_type","cloud_provider_id","commit_hash","creation_time_utc_unix_ms","gpu_vram_remaining_kb" "instance_name","last_updated_utc_unix_ms","mandelbox_capacity","memory_remaining_kb" "nanocpus_remaining","status") VALUES ('test-ip-addr','us-east-1' 'test-ami','g4dn.xlarge','test-aws-id','<the commit hash you are on>','1633366678649','15472256','<your instance name>','1633373119691','1' '31577212','7997643688','PRE_CONNECTION');
```

Make sure to change `<your instance name>` on the command to your actual instance name, `<the commit hash you are on>` to the actual commit hash, and verify the status is `PRE_CONNECTION`.

Once you have added the row to the database, start the host service with `make runlocaldevwithdb` and verify it starts the Hasura subscriptions and enters the event loop correctly.

After this, if you want to test the `SpinUpMandelbox` function by "allocating" a mandelbox for your instance on the local database. Do this by adding a row to the `mandelbox_info` table with this command:

```
INSERT INTO "cloud"."mandelbox_info" ("mandelbox_id","user_id","instance_name","status","creation_time_utc_unix_ms","session_id") VALUES ('182cas8923jnckku023ind3','localdev_user_id','<your instance name>','ALLOCATED','1634241280','4389789vnuf2f32f3f');
```

Again, verify that `<your instance name>` is your actual instance name, and that the status is `ALLOCATED`.

When you "allocate" a mandelbox for your instance on your database, the pubsub should immediately detect the change and fire the `SpinUpMandelbox` function, and then you can test whatever you need.

To test the `DrainAndShutdown` function, which is also tied to the pubsub, change the status of your instance to `DRAINING` on your local database, this should immediately fire the shutdown and cancel the context.

##### Test the pubsub with development database

If, for some reason, you need to test the pubsub with the dev Hasura server, you don't need to do the local setup. Just run follow the same steps for adding your instance and allocating a mandelbox on the development database (change the test values to something real).

Before starting the host service export your heroku api token `export HEROKU_API_TOKEN=<your api token>` and the logzio token `export LOGZIO_SHIPPING_TOKEN=<logzio shipping token>`, and then set the prod logging as necessary `export USE_PROD_LOGGING=false`, then you can start the host service with `make rundev`.

### Design Decisions

This service will not restart on crash/panic, since that could lead to an inconsistency between the actually running mandelboxes and the data left on the filesystem. Instead, we note that if the service crashes, no new mandelboxes will be able to report themselves to the Whist webserver(s), meaning there will be no new connections to the EC2 host and once all running mandelboxes are disconnected, the instance will be spun down.

We never use `os.exit()` or any of the `log.fatal()` variants, or even plain `panic()`s, since we want to send out a message to our Whist webserver(s) and/or Sentry upon the death of this service (this is done with a `defer` function call, which runs after `logger.Panic()` but not after `exit`).

For more details, see the comments and deferred cleanup function at the beginning of `main()`.

We use [contexts](https://golang.org/pkg/context/) from the Go standard library extensively to scope requests and set timeouts. [Brushing up on contexts](https://blog.golang.org/context) might prove useful.

## Styling

We use `goimports` and `golint` for Golang linting and coding standards. These are automatically installed when running `make format` or `make lint`.

The easiest way to check that your code is ready for review (i.e. is linted, vetted, and formatted) is just to run `make check`. This is what we use to check the code in CI as well.

## Publishing

The Whist host service gets built into our AMIs during deployment.

For testing, you can also use the `upload` target in the makefile, which builds a host service and pushes it to the `fractal-host-service` s3 bucket with value equal to the branch name that `make upload` was run from.

## Getting to know the codebase

The best way to learn about the codebase without worrying about implementation details is to browse the [Host Service Documentation](https://docs.whist.com/host-service/). Once you've gotten a high-level overview of the codebase, take a look at the Makefile to understand it, and then start reading through the codebase and writing some code yourself! If something seems confusing to you, feel free to reach out to @djsavvy, and once you figure it out, make a PR to comment it in the code!
