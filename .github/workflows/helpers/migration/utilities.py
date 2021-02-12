import stopit
import config
import time
from functools import wraps

def with_timeout(secs):
    """Returns a decorator that passes its argument to a function"""
    def with_timeout_decorator(func):
        @wraps(func)
        def with_timeout_wrapper(*args, **kwargs):
            return func(secs, *args, **kwargs)
        return with_timeout_wrapper
    return with_timeout_decorator


@with_timeout(config.TIMEOUT_SECONDS)
def ensure_ready(timeout_secs, *fns):
    """Wait for a set of predicates to be true, subject to a timeout

    Given a timeout in functions and a variable number of predicate functions,
    runs the predicate functions over and over until they all return true.

    If timeout_secs elapses first, then a TimeoutError is raised.
    """
    with stopit.ThreadingTimeout(timeout_secs) as t:
        while True:
            time.sleep(0.1)
            if all(f() for f in fns):
                return True
    raise TimeoutError


