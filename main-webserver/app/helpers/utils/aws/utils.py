"""Miscellaneous helper utilities that might make boto3 a little less terrible to use."""

import time


class Retry(Exception):
    """Raised by a function wrapped by the `retry_with_backoff` decorator to trigger a retry."""


def retry_with_backoff(*, min_wait, wait_delta, max_wait, max_retries=None):
    """Call the decorated function, retrying up to the specified number of times.

    Decorated functions may raise Retry to indicate that the function should be retried. The
    __cause__ attribute on retry exception should be set either automatically with "raise from" or
    explicitly with attribute access.

    For example:

        @retry_with_backoff(min_wait=1, wait_delta=1, max_wait=3, max_retries=4)
        def retry_me():
            try:
                # Do some stuff.
            except Exception as exc:
                # Using "raise from" sets the __cause__ attribute on the Retry exception
                # automatically.
                raise Retry from exc

    or:

        @retry_with_backoff(min_wait=1, wait_delta=1, max_wait=3, max_retries=4)
        def retry_me_too():
            try:
                # Do some stuff.
            except Exception as exc:
                # Save any exception that is raised to a variable.
                exception = exc
            else:
                exception = None

            if exception is not None:
                # Set the __cause__ attribute on the instance of the Retry attribute manually.
                retry = Retry()
                retry.__cause__ = exception

                raise retry

    See https://www.python.org/dev/peps/pep-3134/#explicit-exception-chaining for more information.

    Args:
        min_wait: A floating point value representing the smallest number of seconds for which to
            wait between tries. In practice, this is how long to waitbefore the first retry.
        wait_delta: A floating point value representing the amount of time by which to increase the
            wait time before retrying for each retry after the first one.
        max_wait: A floating point value representing the maximum amount of time for which to wait
            between tries. The wait time between ttries will not increase past this value.
        max_retries: Optional. An integer representing the maximum number of times to retry the
            function call. The maximum number of times the function may be called is max_retries +
            1. Setting max_retries=None removes the cap on the maximum number of
            times that this function may be called. Use with care!

    Returns:
        A decorator that calls the function it decorates up to max_retries + 1 times, waiting for
        a period of time, which varies proportionally to the number of retries, between attempts.
    """

    def decorator(func):
        def decorated(*args, **kwargs):
            retries = 0

            while True:
                try:
                    return func(*args, **kwargs)
                except Retry as retry:
                    # Raise the underlying error if we've already made the maximum allowed number
                    # of attempts to call the function successfully.
                    if max_retries is not None and retries >= max_retries:
                        raise retry.__cause__

                    # The wait time starts at min_wait for the first retry and increases by
                    # wait_delta for each subsequent retry up to max_wait.
                    time.sleep(min(min_wait + wait_delta * retries, max_wait))

                retries += 1

        return decorated

    return decorator
