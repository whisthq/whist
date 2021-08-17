#!/usr/bin/env python3

"""entrypoint.py

Print environment configuration values based on the context in which the code is being run. The
context is specified in the form of a git ref that is passed as a command line argument.

Example usage:

    $ ./entrypoint.py dev
    ::set-output name=environment::dev
    ...
    $ ./entrypoint.py refs/heads/my-feature
    ::set-output name=environment::dev
    ...
    $ ./entrypoint.py refs/heads/prod
    ::set-output name=environment::prod
    ...

"""

import sys


def set_environment(ref: str) -> None:
    """Print environment configuration values.

    The configuration values to be printed depend on the git ref specified as the only argument.

    Arguments:
        ref: A git ref that is used to determine which configuration values should be printed.

    Returns:
        None
    """

    if ref.endswith("prod"):
        environment = "prod"
        domain = "auth.fractal.co"
        client_id = "dnhVmqdHkF1O6aWwakv7jDVMd5Ii6VfX"
    elif ref.endswith("staging"):
        environment = "staging"
        domain = "fractal-staging.us.auth0.com"
        client_id = "2VZTg2ZT1DNUr6JquMfeezRXCc8V1nC5"
    else:
        environment = "dev"
        domain = "fractal-dev.us.auth0.com"
        client_id = "6cd4nskyIHQOePVM6q7FU3x7i5sZLwl1"

    print(f"::set-output name=environment::{environment}")
    print(f"::set-output name=auth0-domain::{domain}")
    print(f"::set-output name=auth0-client-id::{client_id}")
    print(f"::set-output name=auth0-client-secret-key::auth0_gha_client_secret_{environment}")


if __name__ == "__main__":
    # Call the main set_environment function with the arguments specified on the command line as
    # positional arguments. If too many or too few arguments were specified, TypeError will be
    # raised.
    set_environment(*sys.argv[1:])  # pylint: disable=no-value-for-parameter
