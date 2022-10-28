#!/usr/bin/env python3

"""entrypoint.py

Write environment configuration values to the $GITHUB_OUTPUT file based on the context in which
the code is being run. The context is specified in the form of a git ref that is passed as a
command line argument.

Example usage:

    $ ./entrypoint.py dev
    # Writes environment=dev to $GITHUB_OUTPUT
    ...
    $ ./entrypoint.py refs/heads/my-feature
    # Writes environment=dev to $GITHUB_OUTPUT
    ...
    $ ./entrypoint.py refs/heads/prod
    # Writes environment=prod to $GITHUB_OUTPUT
    ...

"""

import sys
import os


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
        domain = "auth.whist.com"
        client_id = "K60rqZ4sqUJaXUol4SOjysPUh7OjayQE"
    elif ref.endswith("staging"):
        environment = "staging"
        domain = "fractal-staging.us.auth0.com"
        client_id = "anDOA3nAkJEPnE2YwwUY0MYnJRaXxvcV"
    else:
        environment = "dev"
        domain = "fractal-dev.us.auth0.com"
        client_id = "zfNXAien2yNOJBKaxcVu7ngWfwr6l2eP"

    with open(os.environ["GITHUB_OUTPUT"], "a") as out:
        out.write(f"environment={environment}\n")
        out.write(f"auth0-domain={domain}\n")
        out.write(f"auth0-client-id={client_id}\n")
        out.write(f"auth0-client-secret-key=auth0_gha_client_secret_{environment}\n")


if __name__ == "__main__":
    # Call the main set_environment function with the arguments specified on the command line as
    # positional arguments. If too many or too few arguments were specified, TypeError will be
    # raised.
    set_environment(*sys.argv[1:])  # pylint: disable=no-value-for-parameter
