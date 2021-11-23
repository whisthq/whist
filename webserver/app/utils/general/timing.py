from functools import wraps
import time
from typing import Any, Callable, TypeVar

from app.utils.general.logs import whist_logger

_ReturnType = TypeVar("_ReturnType")


def log_time(func: Callable[..., _ReturnType]) -> Callable[..., _ReturnType]:
    @wraps(func)
    def wrapper(*args: Any, **kwargs: Any) -> _ReturnType:
        start_time = time.time() * 1000
        result = func(*args, **kwargs)
        end_time = time.time() * 1000
        whist_logger.debug(f"{func.__name__} took {end_time - start_time} ms to execute.")
        return result

    return wrapper
