import json
import logging

from functools import wraps

from flask import current_app, jsonify, request
from flask_jwt_extended import get_jwt_identity

from app.constants.http_codes import UNAUTHORIZED
from app.helpers.utils.general.logs import fractal_log


def fractal_auth(func):
    """Authenticates users for requests (a decorator). It checks that the provided username is
    matching the jwt provided (which this server gave to that user on login with a secret key).
    It also authenticates the admin user.

    Args:
        func (function): A function that we are wrapping with this decorator. Read the python docs on
        decorators for more context (effectivelly, this injects code to run before the function and may
        return early if the user is not authenticated, for example).

    Returns:
        function: The wrapped function (i.e. a function that does the provided func if the username was
        authenticated and otherwise returns an unauthorized error).
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
            fractal_log(
                function="",
                label="",
                logs="Bearer error: {error}".format(error=str(e)),
                level=logging.ERROR,
            )
            return (
                jsonify({"error": "No username provided, cannot authorize Bearer token."}),
                UNAUTHORIZED,
            )

        current_user = get_jwt_identity()

        if (
            current_user != username
            and current_app.config["DASHBOARD_USERNAME"] not in current_user
        ):
            format = "%(asctime)s %(levelname)-8s [%(filename)s:%(lineno)d] %(message)s"

            logging.basicConfig(format=format, datefmt="%b %d %H:%M:%S")
            logger = logging.getLogger(__name__)
            logger.setLevel(logging.DEBUG)

            logger.info(
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


def admin_required(func):
    """A decorator that will return unauthorized if the invoker is no the admin user (i.e.
    "DASHBOARD_USERNAME" user). This user can be found in the config db. To log in as this user
    you will have to use the admin login endpoint (which is different from the reguar login endpoint).

    Args:
        func (function): A function that will be wrapped by this decorator and only run if
        an admin is invoking it.

    Returns:
        function: A wrapper that will return unauthorized if the jwt identity provided is not
        an admin and otherwise run the wrapped function.
    """
    @wraps(func)
    def wrapper(*args, **kwargs):
        current_user = get_jwt_identity()

        if current_app.config["DASHBOARD_USERNAME"] not in current_user:
            return (
                jsonify(
                    {
                        "error": (
                            "Authorization failed. Provided username does not match the username "
                            "associated with the provided Bearer token."
                        ),
                    }
                ),
                UNAUTHORIZED,
            )

        return func(*args, **kwargs)

    return wrapper


def developer_required(func):
    """Similar decorator to admin_required, but it allows @tryfractal.com users as well. It is meant
    for developer endpoints such as the test_endpoint where we, for ease of testing, prefer to let any
    of the fractal members connect. It will return unauthorized for non-developers.

    Args:
        func (function): The function that is being decorated.

    Returns:
        function: A wrapper that runs some preprocessing/decoration code and then calls the func in
        certain cases (or returns something else). Check the docks on python decorators for more context.
    """
    @wraps(func)
    def wrapper(*args, **kwargs):
        current_user = get_jwt_identity()
        if not (
            current_app.config["DASHBOARD_USERNAME"] in current_user
            or "@tryfractal.com" in current_user
        ):
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
