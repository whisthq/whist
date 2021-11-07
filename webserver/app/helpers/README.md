# `helpers`

This module contains arguably the most important parts of our web server implementation. As they are currently written, our Flask endpoints are just wrappers around helper function calls, which happen to reside in this module.

There are three things you should know about this module before you try to use it:

1. **It must be installed to your Python virtual environment for tests to pass.** The line `-e app/helpers` in `requirements-test.txt` takes care of this. It's worth noting that this module, which is also a package, by no means stands alone from the rest of the repository; not only does the rest of the Flask application depend on this package, but also this package depends on code in the rest of the Flask application. **You don't have to worry about this if you just run `pip3 install -r requirements-test.txt`.**

2. When trying to locate a particular helper function it is helpful to know two things:

   - The `aws` subdirectory contains the aforementioned helper functions around which the Flask application endpoints are wrappers. For example, the `/mandelbox/ping` endpoint is a wrapper around a function called `pingHelper`, which performs a very specific task in response to a ping from the protocol server running in a streamed application's mandelbox. Unsurprisingly, the `pingHelper` function will only ever be called from the implementation of the `/mandelbox/ping` endpoint. Functions like `pingHelper` can be found in the `aws` subdirectory. The helper function depends on the `utils/` directory.

3. `git grep` is useful.
