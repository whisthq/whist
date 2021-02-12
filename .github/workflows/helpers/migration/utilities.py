import stopit
import config
from functools import wraps

def with_timeout(secs):
    def with_timeout_decorator(func):
        def with_timeout_wrapper(*args, **kwargs):
            return func(secs, *args, **kwargs)
        return with_timeout_wrapper
    return with_timeout_decorator


@with_timeout(config.TIMEOUT_SECONDS)
def ensure_ready(timeout_secs, *fns):
    with stopit.ThreadingTimeout(timeout_secs) as t:
        while True:
            if all(f() for f in fns):
                return True
    raise TimeoutError


