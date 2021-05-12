from functools import wraps

from flask import abort, jsonify, request
from flask_jwt_extended import get_jwt_identity

from app.models import User

from app.constants.http_codes import UNAUTHORIZED, PAYMENT_REQUIRED
from app.helpers.utils.general.logs import fractal_logger
from auth0 import has_scope, scope_required

developer_required = scope_required("admin")


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
        if not has_scope("admin"):
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
