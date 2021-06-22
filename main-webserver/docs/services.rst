.. services.rst
   A description of the other internal and external services with which the
   Fractal webserver communicates.

Other Services
==============


User Agents
-----------

The Fractal Desktop Applications allow customers to register and log into user accounts. Once a Fractal Desktop Application
is authenticated, it requests that the Fractal Webserver allocates it a Fractal Container at which point a 
streaming session can begin. Developers use tools such as web browsers, Postman, and cURL to perform manual tests against 
instances of the Fractal Webserver, and also to perform administrative actions like creating and deleting cloud resources.


Fractal Host Service
--------------------

The Fractal Host Service is responsible for orchestrating container allocation and interaction on a host machine. 
It listens for connections from the Fractal Webserver, which periodically sends commands and data that influence how and when 
containerized application streaming sessions start and run to the Fractal Host Service. It is also responsible for handling 
properly setting up Fractal Containers and their interaction with the host, notably via allocating TTYs and Uinput nodes. The 
Fractal Host Service also sends periodic heartbeats back to the Fractal Webserver to let it know that it is alive.


Fractal Protocol Server
-----------------------

The Fractal Protocol Server is responsible for handling the server-side part of the streaming, and is installed in Fractal
Containers. It notifies the Fractal Webserver when connected clients disconnect, so that the Fractal Webserver knows when
to tell AWS to delete instances/containers that are no longer needed.


GitHub Actions Workflows
------------------------

Each time a new release is deployed, a GitHub Actions workflow places the Fractal webserver in maintenance mode so that the
deployment can happen atomically. It is necessary to suspend normal Webserver operation temporarily when deploying new 
releases so that the system has time to programmatically update all of our cloud infrastructure to that of the new release.


HireFire
--------

HireFire is a Heroku add-on that is responsible for scaling our Celery worker processes. The HireFire service periodically 
requests scaling metrics from the Fractal webserver. It uses these metrics to determine the desired number of Celery worker 
processes to run and scales the actual number of running Celery worker processes accordingly.


Databases
---------

On startup, the Fractal Webserver reads from a centralized database of Flask configuration variables that may be overridden 
by Heroku environment variables. We call this database the configuration database and it is shared by all Fractal Webserver 
deployments. Each Fractal Webserver deployment is also attached to an all-purpose PostgresSQL database that it uses at 
runtime. This database keeps track of streaming sessions that have been initiated by API requests to the associated 
Fractal Webserver.


AWS Services
------------

NOTE: This segment describes the state of the system *post* refactor -- pre-refactor the system is similarly structured but interacts more heavily with several more AWS abstractions
The web server is responsible for scaling cloud resources. In particular, it interacts heavily with AWS EC2
Specifically, the webserver has a utility library interface over the AWS EC2 API that allows us to provision and deprovision cloud resources from python code easily
We use this utility library when scaling up (adding more) or scaling down (shutting off) ec2 instances due to periods of lower or higher load.
Once active, these instances are interfaced with via the ECS Host Service (see above) rather than directly via the EC2 API whenever possible. This invariant is only violated when the host service is malfunctioning, at which point the utility library is used to directly shut down instances.


Stripe
------

The Fractal Webserver is responsible for associating Stripe customers with Fractal user accounts, managing subscriptions, and 
granting or denying users service based on their subscription status.
