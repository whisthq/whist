import threading
from app.helpers.utils.general.logs import fractal_logger

# global lock-protected variable indicating whether webserver can process web requests
_WEB_REQUESTS_ENABLED = True
_WEB_REQUESTS_LOCK = threading.Lock()


def set_web_requests_status(enabled: bool) -> bool:
    """
    Set state of _WEB_REQUESTS_ENABLED.
    Args:
        enabled: State of _WEB_REQUESTS_ENABLED. If True, web requests allowed. If False, all are
            rejected with WEBSERVER_MAINTENANCE status code.
    Returns:
        True iff _WEB_REQUESTS_ENABLED was set to enabled.
    """
    global _WEB_REQUESTS_ENABLED  # pylint: disable=global-statement
    has_lock = _WEB_REQUESTS_LOCK.acquire(blocking=True, timeout=1)
    if not has_lock:
        # this should not happen and means our lock contention is bad
        fractal_logger.error(
            f"Could not acquire web requests lock. Could not set _WEB_REQUESTS_ENABLED={enabled}."
        )
        return False
    _WEB_REQUESTS_ENABLED = enabled
    _WEB_REQUESTS_LOCK.release()
    fractal_logger.info(f"_WEB_REQUESTS_ENABLED set to {enabled}")
    return True


def can_process_requests() -> bool:
    """
    Get state of _WEB_REQUESTS_ENABLED in thread-safe manner.

    Returns:
        True iff _WEB_REQUESTS_ENABLED is True. False if _WEB_REQUESTS_ENABLED is False
        or acquiring lock failed.
    """
    has_lock = _WEB_REQUESTS_LOCK.acquire(blocking=True, timeout=1)
    if not has_lock:
        # this should not happen and means our lock contention is bad
        fractal_logger.error(
            "Could not acquire web requests lock. Assuming requests cannot be handled."
        )
        return False
    # copy value into web_requests_status
    web_requests_status = _WEB_REQUESTS_ENABLED
    _WEB_REQUESTS_LOCK.release()
    return web_requests_status