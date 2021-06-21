.. services.rst
   A description of the other internal and external services with which the
   Fractal webserver communicates.

Other Services
==============


User Agents
-----------

* The Fractal desktop applications allow customers to register and log into user accounts. Once a Fractal desktop application
* is authenticated, it requests that the Fractal webserver allocates it a Fractal container/mandelbox, at which point a 
* streaming session can begin. Developers use tools such as web browsers, Postman, and cURL to perform manual tests against 
* instances of the Fractal webserver, and also to perform administrative actions like creating and deleting cloud resources.


The Host Service
----------------

* The Fractal host service is responsible for orchestrating container allocation and interaction on a host machine. It listens
* for connections from the webserver, which periodically sends commands and data that influence how and when containerized 
* application streaming sessions start and run to the Fractal host service. It is also responsible for handling properly
* setting up Fractal containers and their interaction with the host, notably via allocating TTYs and Uinput nodes. The 
* Fractal host service also sends periodic heartbeats back to the webserver to let it know that it is alive.


The Fractal Protocol Server
---------------------------

* The Fractal protocol server is responsible for handling the server-side part of the streaming, and is installed in Fractal
* containers. It notifies the Fractal webserver when connected clients disconnect, so that the Fractal webserver knows when
* to tell AWS to delete containers that are no longer needed.


GitHub Actions Workflows
------------------------

Each time a new release is deployed, a GitHub Actions workflow places the web server in maintenance mode so the deployment can happen atomically. It is necessary to suspend normal Webserver operation temporarily when deploying new releases to give us time to update all of our cloud infrastructure.


HireFire
--------

HireFire is a Heroku add-on that is responsible for scaling our Celery worker processes. The HireFire service periodically requests scaling metrics from the web server. It uses these metrics to determine the desired number of Celery worker processes to run and scales the actual number of running Celery worker processes accordingly.


Databases
---------

On startup, the web server reads from a centralized database of Flask configuration variables that may be overridden by Heroku environment variables. We call this database the configuration database and it is shared by all Fractal Webserver deployments. Each Webserver deployment is also attached to an all-purpose PostgreSQL database that it uses at runtime. This database keeps track of streaming sessions that have been initiated by API requests to the associated Webserver.


AWS services
------------

NOTE: This segment describes the state of the system *post* refactor -- pre-refactor the system is similarly structured but interacts more heavily with several more AWS abstractions
The web server is responsible for scaling cloud resources. In particular, it interacts heavily with AWS EC2
Specifically, the webserver has a utility library interface over the AWS EC2 API that allows us to provision and deprovision cloud resources from python code easily
We use this utility library when scaling up (adding more) or scaling down (shutting off) ec2 instances due to periods of lower or higher load.
Once active, these instances are interfaced with via the ECS Host Service (see above) rather than directly via the EC2 API whenever possible. This invariant is only violated when the host service is malfunctioning, at which point the utility library is used to directly shut down instances.


Stripe
------

The web server is responsible for associating Stripe customers with Fractal user accounts, managing subscriptions, and granting or denying users service based on their subscription status.
