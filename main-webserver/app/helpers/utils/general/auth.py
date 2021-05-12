import json

from functools import wraps

from flask import abort, jsonify, request
from flask_jwt_extended import get_jwt_identity

from app.models import User

from app.constants.http_codes import UNAUTHORIZED, PAYMENT_REQUIRED
from app.helpers.utils.general.logs import fractal_logger
from auth0 import has_scope, scope_required

developer_required = scope_required("admin")


def fractal_auth(func):
    """Authenticates users for requests (a decorator). It checks that the
    provided username is matching the jwt provided (which this server gave to that
    user on login with a secret key). It also authenticates the admin user.

    Args:
        func (function): A function that we are wrapping with this decorator.
        Read the python docs on decorators for more context (effectivelly, this injects
        code to run before the function and may return early if the user is not authenticated,
        for example).

    Returns:
        function: The wrapped function (i.e. a function that does the
        provided func if the username was authenticated and otherwise returns an
        unauthorized error).
    """

    @wraps(func)
    def wrapper(*args, **kwargs):
        username = None

        try:
            if request.method == "POST":
                body = json.loads(request.data)
                if "username" in body.keys():
                    username = body["username"]
                elif "email" in body.keys():
                    username = body["email"]
            elif request.method == "GET":
                username = request.args.get("username")
        except Exception as e:
            fractal_logger.error(
                "Bearer error: {error}".format(error=str(e)),
            )
            return (
                jsonify({"error": "No username provided, cannot authorize Bearer token."}),
                UNAUTHORIZED,
            )

        current_user = get_jwt_identity()

        if username is None or (current_user != username and not has_scope("admin")):
            fractal_logger.info(
                f"Authorization failed. Provided username {username} does not match username "
                f"associated with provided Bearer token {current_user}."
            )
            return (
                jsonify(
                    {
                        "error": (
                            f"Authorization failed. Provided username {username} does not match "
                            f"the username associated with provided Bearer token {current_user}."
                        )
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
