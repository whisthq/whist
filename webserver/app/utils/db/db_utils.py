"""
Contains helpers for our SQLAlchemy db connection. Specifically:
- set_local_lock_timeout
"""


from app.models import db


def set_local_lock_timeout(seconds: int):
    """
    Call this right before a SQLAlchemy query that uses with_for_update() to put a timeout on the
    locking mechanism. Example (with error handling):

    from sqlalchemy.exc import OperationalError
    try:
        # this gets the all unassigned mandelboxes with a 10 second timeout on the lock
        set_local_lock_timeout(10)
        unassigned_mandelboxes = MandelboxInfo.query.with_for_update().filter_by(user_id=None)
    except OperationalError as oe:
        if "lock timeout" in str(oe):
            # handle lock timeout
            pass
        else:
            # any other error raise again
            raise oe

    Args:
        seconds: seconds to try getting lock before timing out
    """
    db.session.execute(f"SET LOCAL lock_timeout='{seconds}s';")
