.. Whist Webserver documentation main file, created by
   sphinx-quickstart on Mon Apr 12 16:38:37 2021.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

Welcome to the Whist Webserver internal documentation!
========================================================

.. toctree::
   :maxdepth: 2
   :caption: Contents:

   authentication
   responsibilities
   stripe
   services
   modules


Introduction
------------

The Whist Webserver is responsible for implementing all of Whist's business logic. Its responsibilities include:

* Access control (lightweight authentication by interfacing with Auth0, our authentication provider)
* Interfacing our frontend (end user applications) with our cloud infrastructure
* Streaming sessions management (monitoring active instances, etc.)

In order to fulfill all of its responsibilities, the Whist Webserver communicates with many other internal and external
services. It receives incoming requests from:

* User agents (e.g. the desktop applications, browsers, developer tools such as Postman and cURL)
* The Whist Host Service running on Whist cloud instances
* Instances of the Whist protocol server running in Whist mandelboxes on Whist cloud instances
* GitHub Actions (GHA) workflows, for deployment and testing

It also sends outgoing queries to:

* Whist Databases
* Various AWS services used by Whist
* The Whist Host Service running on Whist cloud instances
* Auth0, the authentication provider used by Whist
* Stripe, the payment provider used by Whist


Indices & Tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
