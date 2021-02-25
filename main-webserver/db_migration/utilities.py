import time
from functools import wraps
from pathlib import Path
import stopit
import config


def is_ignored_head(ln: str):
    """Test if a string begins with a IGNORE prefix.

    If a string starts with a substring that is in the set config.IGNORE_HEAD,
    it should be ignored.

    Args:
        ln: A string.
    Returns:
        True if the string starts with a substring in config.IGNORE_HEAD,
        False otherwise.
    """
    for head in config.IGNORE_HEAD:
        if ln.startswith(head):
            return True
    return False


def is_ignored_complete(ln: str):
    """Test if a string is in the set of ignored statements.

    Args:
        ln: A string.
    Returns:
        True if the string is in the set config.IGNORE_COMPLETE,
        False otherwise.
    """
    return ln in config.IGNORE_COMPLETE


def ignore_filter(lines):
    """Filters pre-configured lines from a string of SQL commands

    Certain SQL statements (like ones that generate random values) will always
    appear in the SQL schema diff. This function filters pre-configured SQL
    strings from the diff output.

    Args:
        lines: A list of strings to filter, based on IGNORE configuration.
    Returns:
        A list of strings, with statements to IGNORE filtered out.
    """
    no_empty = (ln for ln in lines if ln)
    ignored_head = (ln for ln in no_empty if not is_ignored_head(ln))
    ignored_complete = (ln for ln in ignored_head if not is_ignored_complete(ln))
    return list(ignored_complete)


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
