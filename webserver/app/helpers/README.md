# `helpers`

This module contains arguably the most important parts of our web server implementation. As they are currently written, our Flask endpoints are just wrappers around helper function calls, which happen to reside in this module. This module also contains helpful utility functions for performing tasks such as logging.

There are three things you should know about this module before you try to use it:

1. **It must be installed to your Python virtual environment for tests to pass.** The line `-e app/helpers` in `requirements-test.txt` takes care of this. It's worth noting that this module, which is also a package, by no means stands alone from the rest of the repository; not only does the rest of the Flask application depend on this package, but also this package depends on code in the rest of the Flask application. **You don't have to worry about this if you just run `pip install -r requirements-test.txt`.**
2. When trying to locate a particular helper function it is helpful to know two things:

    - functions that are called more than once will be found in the `utils` subdirectory, while functions that are only used once are likely to be found in the `blueprint_helpers` subdirectory.
      The `blueprint_helpers` subdirectory contains the aforementioned helper functions around which the Flask application endpoints are wrappers. For example, the `/mandelbox/ping` endpoint is a wrapper around a function called `pingHelper`, which performs a very specific task in response to a ping from the protocol server running in a streamed application's mandelbox. Unsurprisingly, the `pingHelper` function will only ever be called from the implementation of the `/mandelbox/ping` endpoint. Functions like `pingHelper` can be found in the `blueprint_helpers` subdirectory.

        The `utils` subdirectory, on the other hand, contains functions that might be more generally useful, at least within a particular group of endpoints. The functions defined in this subdirectory define functions that aren't necessarily wrapped by a Flask application endpoint. They are usually called from within at least one of the helper functions in the `blueprint_helpers` subdirectory. For example, multiple helper functions in `blueprint_helpers/aws` depend on the `ECSClient` class defined in `utils/aws/base_ecs_client.py`. Additionally the webserver has a rate limiter, located under `utils/general/limiter.py`, that restricts our public-facing routes to accept only 10 requests per minute to prevent DDoS attacks.

    - The directory structures of both the `blueprint_helpers` and `utils` subdirectories closely resemble that of the `app/blueprints` directory.
      If you're looking for a helper function that is called by many of the mandelbox management endpoints, it's probably going to be defined in `utils/aws`.

3. `git grep` is useful.
