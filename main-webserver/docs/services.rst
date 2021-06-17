.. services.rst
   A description of the other internal and external services with which the
   Fractal web server communicates.

Other services
==============


User agents
-----------

* The Fractal desktop application allows customers to register and log into user accounts. Once the desktop application is authenticated, it can request that the web server allocate it a streaming session.
* Developers use tools such as web browsers, Postman, and cURL to perform manual tests against instances of the web server and also to perform administrative actions like creating and deleting cloud resources.


The ECS host service
--------------------

* The ECS host service wraps Amazon's default ECS agent and extends its functionality.
* It listens for connections from the web server, which periodically sends commands and data to the host service that influence how and when containerized application streaming sessions start and run.
* The ECS host service also sends periodic heartbeats back to the web server to let it know that it is alive.


The Fractal protocol server
---------------------------------

* The Fractal protocol server notifies the web server when connected clients disconnect so the web server knows when to tell AWS to delete containers that are no longer needed.


GHA workflows
-------------

* Each time a new release is deployed, a GitHub Actions workflow places the web server in maintenance mode so the deployment can happen atomically. It is necessary to suspend normal Webserver operation temporarily when deploying new releases to give us time to update all of our cloud infrastructure.


HireFire
--------

* HireFire is a Heroku add-on that is responsible for scaling our Celery worker processes.
* The HireFire service periodically requests scaling metrics from the web server. It uses these metrics to determine the desired number of Celery worker processes to run and scales the actual number of running Celery worker processes accordingly.


Databases
---------

* On startup, the web server reads from a centralized database of Flask configuration variables that may be overridden by Heroku environment variables. We call this database the configuration database and it is shared by all Fractal Webserver deployments.
* Each Webserver deployment is also attached to an all-purpose PostgreSQL database that it uses at runtime. This database keeps track of streaming sessions that have been initiated by API requests to the associated Webserver.


AWS services
------------

* The web server is responsible for scaling cloud resources. In particular, it interacts heavily with AWS ECS, EC2, and AutoScaling.


Stripe
------

* The web server is responsible for associating Stripe customers with Fractal user accounts, managing subscriptions, and granting or denying users service based on their subscription status.
