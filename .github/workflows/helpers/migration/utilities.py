import stopit
import config
import time
from functools import wraps


def with_timeout(func):
    """A decorator that passes a configured timeout value to a function

    Uses the timeout value stored in config.TIMEOUT_SECONDS.

    Args:
        func: A function that accepts a TIMEOUT_SECONDS number as its first
              argument.
    Returns:
        The input function partially applied with the timeout configuration.
    """

    @wraps(func)
    def with_timeout_wrapper(*args, **kwargs):
        return func(config.TIMEOUT_SECONDS, *args, **kwargs)

    return with_timeout_wrapper


@with_timeout
def ensure_ready(timeout_secs, *fns):
    """Wait for a set of predicates to be true, subject to a timeout.

    Given a timeout in functions and a variable number of predicate functions,
    runs the predicate functions over and over until they all return true.

    If timeout_secs elapses first, then a TimeoutError is raised.

    Args:
        timeout_secs: A number representing the number of seconds to wait
                      before raising a TimeoutError.
        fns: A variable number of fns that should return True or False.

    Raises:
        A TimeoutError if timeout_secs elapses before all the predicate
        functions return True.
    """
    with stopit.ThreadingTimeout(timeout_secs) as t:
        while True:
            time.sleep(0.1)
            if all(f() for f in fns):
                return True
    raise TimeoutError
