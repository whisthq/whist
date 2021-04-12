.. Fractal documentation master file, created by
   sphinx-quickstart on Mon Apr 12 16:38:37 2021.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

Welcome to Fractal's internal documentation!
============================================

.. toctree::
   :maxdepth: 2
   :caption: Contents:

   architecture
   responsibilities
   services


Introduction
------------

Fractal's Webserver is responsible for implementating all of Fractal's business logic. Its responsibilities include

* access control, and
* streaming session management.

In order to fulfill all of its responsibilites, Fractal's Webserver communicates with may other internal and external services. It receives incoming requests from

* user agents (e.g. the desktop application, browsers, developer tools such as Postman and cURL),
* the ECS host service,
* Fractal protocol server instances,
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
