# Whist Backend

This repository contains all the projects related to the Whist backend. They are divided into the following categories:

- Authentication: `/auth0`. This is the third-party authentication service that we use to authenticate our users.

- Infrastructure: `/infrastructure`. This is the Terraform configuration that we use to maintain infrastructure-as-code, so that we can easily keep track of our public cloud infrastructure and ensure that it is consistent across regions and cloud providers.

- Services: `/services`. This is the code that implements the business logic of the Whist backend, and is divided into the following categories:

  - `/host-service`: This is the code that implements the Host-Service, which is installed on instances runnings user containers and is responsible for starting and stopping mandelboxes, and making sure that the instances are optimized for Whist. The Host-Service communicates with the database to inform it of when users start and stop mandelboxes, so that users can be dynamically allocated to instances with available capacity.
  - `/scaling-service`: This is the code that implements the Scaling-Service, which is the backend service that is responsible for assigning users to instances, and for dynamically scaling the number of instances to meet demand.
  - All other subfolders are shared utilities used by our backend services.

- Webserver: `/webserver`. This code is getting sunsetted, it is being replaced by the Scaling-Service. It is currently still responsible for performing instance allocation and handling payments, but will soon be fully replaced and removed. Do not modify.

Please refer to each of those projects for their respective development practices, code standards, and further documentation.
