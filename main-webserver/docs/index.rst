.. Fractal documentation main file, created by
   sphinx-quickstart on Mon Apr 12 16:38:37 2021.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

Welcome to the Fractal Webserver internal documentation!
========================================================

.. toctree::
   :maxdepth: 2
   :caption: Contents:

   architecture
   responsibilities
   services


Introduction
------------

The Fractal Webserver is responsible for implementating all of Fractal's business logic. Its responsibilities include:

* access control
* streaming session management


interfacing frontend with cloud infrastructure
passing parameters for streaming sessions


In order to fulfill all of its responsibilites, the Fractal Webserver communicates with many other internal and external 
services. It receives incoming requests from:

* user agents (e.g. the desktop application, browsers, developer tools such as Postman and cURL),
* the Fractal Host Service,
* instances of the Fractal protocol server running in ECS containers,
* GitHub Actions (GHA) workflows, and
* HireFire.

It also sends outgoing queries to

* databases,
* various AWS services,
* the ECS host service,
* Auth0, and
* Stripe.


Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
