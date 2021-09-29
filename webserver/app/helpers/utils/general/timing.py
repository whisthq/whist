from functools import wraps
import time
from typing import Any, Callable, TypeVar

from app.helpers.utils.general.logs import fractal_logger

_ReturnType = TypeVar("_ReturnType")


def log_time(func: Callable[..., _ReturnType]) -> Callable[..., _ReturnType]:
    @wraps(func)
    def wrapper(*args: Any, **kwargs: Any) -> _ReturnType:
        start_time = time.time() * 1000
        result = func(*args, **kwargs)
        end_time = time.time() * 1000
        fractal_logger.debug(f"{func.__name__} took {end_time - start_time} ms to execute.")
        return result

    return wrapper
