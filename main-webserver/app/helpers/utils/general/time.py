import datetime
from functools import wraps
import errno
import os
import signal

from dateutil.relativedelta import relativedelta


def unix_to_date(utc):
    return datetime.datetime.fromtimestamp(utc)


def date_to_string(time):
    return time.strftime("%m/%d/%Y, %H:%M")


def get_current_time():
    return date_to_string(datetime.datetime.now())


def date_to_unix(date):
    return round(date.timestamp())


def get_today():
    aware = datetime.datetime.now()
    return aware


def shift_unix_by_month(utc, num_months):
    date = unix_to_date(utc)
    return round(date_to_unix(date + relativedelta(months=num_months)))


def shift_unix_by_week(utc, num_weeks):
    date = unix_to_date(utc)
    return round(date_to_unix(date + relativedelta(weeks=num_weeks)))


def shift_unix_by_minutes(utc, num_minutes):
    date = unix_to_date(utc)
    return round(date_to_unix(date + relativedelta(minutes=num_minutes)))


# from https://stackoverflow.com/questions/2281850/timeout-function-if-it-takes-too-long-to-finish
class TimeoutError(Exception):
    """
    Generic Timeout class
    """


# from https://stackoverflow.com/questions/2281850/timeout-function-if-it-takes-too-long-to-finish
def timeout(seconds, error_message=os.strerror(errno.ETIMEDOUT)):
    """
    Decorator to make a timeout for long-running functions. This is necessary
    because Python's SSL library seems to not handle handshake timeouts (see:
    https://stackoverflow.com/questions/19938593/web-app-hangs-for-several-hours-in-ssl-py-at-self-sslobj-do-handshake
    )
    Unfortunately, this means we need to do the timeout ourselves.

    Usage:
    @timeout(seconds=NUM_SEC, error_message="Whatever you want")
    def long_running_func()
    """

    def timer_decorator(func):
        def _handle_timeout(signum, frame):
            raise TimeoutError(error_message)

        @wraps(func)
        def wrapper(*args, **kwargs):
            signal.signal(signal.SIGALRM, _handle_timeout)
            signal.alarm(seconds)
            try:
                result = func(*args, **kwargs)
            finally:
                signal.alarm(0)
            return result

        return wrapper

    return timer_decorator
