from app.models import MandelboxInfo


def is_user_active(username: str) -> bool:
    """
    This function determines whether a given user already has a mandelbox running.
    Is it's own function for ease of testing.
    Args:
        username: Which user to check for

    Returns: True if and only if the user has a mandelbox in the DB
    """
    return MandelboxInfo.query.filter_by(user_id=username).one_or_none() is not None
