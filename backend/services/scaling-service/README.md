# Whist Scaling-Service

The Whist Scaling-Service is an event driven system, which is composed of three main parts: the event handler, the scaling algorithm(s), and the host handler. It is uses for scaling up and down host instances based on demand, and assigning users to instances.

It follows the same design conventions as the host-service, but with a more event-focused approach. As a result, it uses Go channels, Goroutines and contexts heavily. It is recommended to be familiar with these concepts while working on the scaling-service. A more detailed write up of the scaling service is available [here](https://www.notion.so/whisthq/4d91593ea0e0438b8bdb14c25c219d55?v=0c3983cf062d4c3d96ac2a65eb31761b&p=1a8d9b668a8442e79778fb41df01c3e8).

## Development

### Overview

#### Event Handler

The event handler can be seen as the main process in the scaling-service. It is responsible of creating and initializing database subscriptions, scheduled events, and scaling algorithms, which are indexed on a sync map. It then receives events, and sends them to the appropiate channels. In a sense, this is the simplest part of the scaling-service.

#### Scaling Algorithm

Scaling algorithms are abstracted on the `ScalingAlgorithm` interface, which has generic methods that every scaling algorithm will need. The intention behind making a simple interface is that each scaling algorithm can be extended as necessary to perform different scaling strategies, so that they can be swaped by the event handler with ease.

Overall, a scaling algorithm has the following structure:

- The `actions` file, which contains code for the scaling actions (scaling up/down). This file is the most important one as the actions make the scaling decisons.
- The `config` file contains constants and variables that are used by the scaling actions, such as the size of the buffer of "waiting instances" and bundled regions.
- The main file (in the default scaling algorithm it is named `default`). Here are the method implementations to satisfy the `ScalingAlgorithm` interface. It also has the logic for starting actions based on the received events.

#### Database Client

This package is the equivalent of the `dbdriver` package from the host-service. It interacts directly with the database by using a Hasura GraphQL client. The intention behind doing a separate package is to abstract all the interactions with the database, such that the scaling algorithm simply calls a function with the necessary parameters and gets the result from the database, without interacting directly with it. The package also contains types necessary for parsing requests/responses from Hasura.

#### Host Handler

Host handlers are abstracted on the `HostHandler` interface, which has the basic methods necessary to perform scaling actions. Each host handler implementation is meant to deal directly with the cloud provider's sdk. By abstracting cloud provider specific logic behind an interface, the scaling actions can be made in an agnostic way, so that multi-cloud support can be added easily. The structure of a host handler is very straightforward , it only has a `config` file with configuration variables specific to the cloud provider, and the main host file where the interface methods are implemented.

### Running the Scaling-Service

The scaling-service can be run locally on your computer with the command `make run_scaling_service`. Make sure to have your AWS credentials configured with `aws configure` so that the scaling-service is able to start/stop instances. Another thing to keep in mind is that for local testing you need to provide a database (by following the steps above) and add valid instance images to it.

## Implemented Scaling Algorithms

The following scaling algorithms are fully implemented on the scaling-service:

1. **Default scaling algorithm**: this is a general solution that works well on any region, and includes all of the functionalities the team has used in the past. It uses an instance buffer to maintain active instances on each region, includes retry logic when launching instances, and atomic image upgrades. It also verifies the correct termination of instances and notifies of any errors. It increments instances one-at-a-time, and decrements them one-at-a-time, which is a slow scaling process and is best suited for small regions/regions with low demand.

## Deployment

The Whist scaling-service gets deployed to Heroku pipeline `whist-server`. Note that our Hasura GraphQL servers are also hosted on Heroku on the Heroku pipeline `whist-hasura`. The logic for deploying can be found in the `deploy.sh` script, which is responsible of generating necessary files and pushing the new changes to Heroku. This logic gets triggered as part of our standard deployment process in the `whist-build-and-deploy.yml` file.

