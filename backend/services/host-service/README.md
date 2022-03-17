# Whist Host-Service

## Development

### Overview

## Deployment

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
