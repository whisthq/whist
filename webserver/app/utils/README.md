# `utils`

This module contains helpful utility functions for performing tasks such as logging.

Functions that are called more than once will be found in this directory. The `utils` directory contains functions that might be more generally useful, at least within a particular group of endpoints. The functions defined in this directory define functions that aren't necessarily wrapped by a Flask application endpoint. Some are called from within at least one of the helper functions in the `aws` directory. For example, multiple helper functions in `aws` depend on the `ECSClient` class defined in `utils/aws/base_ecs_client.py`. Additionally the webserver has a rate limiter, located under `general/limiter.py`, that restricts our public-facing routes to accept only 10 requests per minute to prevent DDoS attacks.

If you're looking for a helper function that is called by many of the mandelbox management endpoints, it's probably going to be defined in `aws/`.

The module also has vital functions in `flask/` which manages creating the flask app, registering the endpoints and much more.
