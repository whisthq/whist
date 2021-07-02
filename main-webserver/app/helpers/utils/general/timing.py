from functools import wraps
import time

from app.helpers.utils.general.logs import fractal_logger


def log_time(func):
    @wraps(func)
    def wrapper(*args, **kwargs):
        start_time = time.time() * 1000
        result = func(*args, **kwargs)
        end_time = time.time() * 1000
        fractal_logger.debug(f"{func.__name__} took {end_time - start_time} ms to execute.")
        return result

    return wrapper
