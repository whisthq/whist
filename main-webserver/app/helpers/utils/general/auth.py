from functools import wraps

from flask import abort, current_app, jsonify, request
from flask_jwt_extended import get_jwt_identity

from app.models import User

from app.constants.http_codes import UNAUTHORIZED, PAYMENT_REQUIRED
from app.helpers.utils.general.logs import fractal_logger


def check_developer() -> bool:
    """Determine whether or not the sender of the current request is a Fractal developer.

    This function must be called with request context. More specifically, it should be called from
    within a view function that is decorated with @jwt_required().

    Returns:
        True iff the requester is a verified Fractal developer or the admin user.

    Raises:
        RuntimeError: This function was called before @jwt_required() and verify_jwt_in_request().
    """

    current_user = get_jwt_identity()
    if current_user is None:
        return False
    elif current_app.config["DASHBOARD_USERNAME"] in current_user:
        return True
    elif current_user.endswith("@fractal.co"):
        # first just check if the user is in the db and verified
        user = User.query.get(current_user)
        if user is not None and user.verified is True:
            return True
        # otherwise, allow a verified developer with the name real_developer to pass
        # real_developer+anything@fractal.co. This lets devs create new accounts that don't have
        # real emails associated with them. According to
        # https://gmail.googleblog.com/2008/03/2-hidden-ways-to-get-more-from-your.html
        # these emails still map to real_developer@fractal.co
        groups = current_user.split("+")
        if len(groups) >= 2:
            # if a + was found in the username, we extract everything before the + and use that
            # as the current user
            real_developer = groups[0]
            current_user = f"{real_developer}@fractal.co"
            user = User.query.get(current_user)
            # make sure the real_developer is actually verified
            return user is not None and user.verified is True
    return False


def developer_required(func):
    """Block unauthorized access to an endpoint.

    Endpoints that are decorated with @developer_required only respond to requests sent by users
    with @fractal.co accounts and to requests sent by the admin user.

    Args:
        func (function): The view function that is being decorated.

    Returns:
        The wrapped view function. The wrapped view function will always respond with a 401
        Unauthorized response to unauthorized requests. It will execute as normally in response
        to properly authorized requests.
    """

    @wraps(func)
    def wrapper(*args, **kwargs):
        if not check_developer():
            return (
                jsonify(
                    {
                        "error": (
                            "Authorization failed. Provided username does not belong to "
                            "a developer account."
                        ),
                    }
                ),
                UNAUTHORIZED,
            )

        return func(*args, **kwargs)

    return wrapper


def payment_required(func):
    """A decorator that will return unauthorized if the invoker is not a valid
        paying user (not on a free trial, and does not have a valid stripe id/subscription).
        Called before a container assign attempt is made - if the user is not valid,
        a container will not be assigned.

    Args:
        func (function): The function that is being decorated.

    Returns:
        function: A wrapper that runs some preprocessing/decoration code and
        then calls the func in certain cases (or returns something else). Check the
        docs on python decorators for more context.
    """

    @wraps(func)
    def wrapper(*args, **kwargs):
        # admin/developer override
        if not check_developer():
            user = User.query.get(get_jwt_identity())

            if user is None:
                abort(UNAUTHORIZED)

            if not user.subscribed:
                fractal_logger.warning(f"{user.user_id} must pay to access {request.path}.")

                return (
                    jsonify(
                        {
                            "error": ("User is not a valid paying user."),
                        }
                    ),
                    PAYMENT_REQUIRED,
                )
        return func(*args, **kwargs)

    return wrapper
