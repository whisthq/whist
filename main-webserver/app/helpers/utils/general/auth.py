import json
import logging
import datetime


from datetime import datetime as dt

from functools import wraps

from flask import current_app, jsonify, request
from flask_jwt_extended import get_jwt_identity

from app.models import User

from app.constants.http_codes import UNAUTHORIZED, PAYMENT_REQUIRED
from app.constants.time import SECONDS_IN_MINUTE, MINUTES_IN_HOUR, HOURS_IN_DAY
from app.helpers.utils.general.logs import fractal_log
from app.helpers.utils.payment.stripe_client import StripeClient


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


def check_developer() -> bool:
    """Determine whether or not the sender of the current request is a Fractal developer.

    This function may only be called from within view functions decorated by the @jwt_required()
    decorator.

    Returns:
        True iff the requester is a Fractal developer or the admin user.

    Raises:
        RuntimeError: This function was either called without Flask application context or before
            calling either @jwt_required() or verify_jwt_in_request().
    """

    current_user = get_jwt_identity()
    if current_user is None:
        return False
    fractal_log("check_developer", None, current_user)
    return current_app.config["DASHBOARD_USERNAME"] in current_user or "@fractal.co" in current_user


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

        current_user = get_jwt_identity()

        # admin override
        if current_app.config["DASHBOARD_USERNAME"] not in current_user:

            user = User.query.get(current_user)
            stripe_client = StripeClient(current_app.config["STRIPE_SECRET"])

            assert user

            customer = stripe_client.user_schema.dump(user)
            stripe_customer_id = customer["stripe_customer_id"]

            time_diff = round(dt.now(datetime.timezone.utc).timestamp()) - user.created_timestamp
            days_since_account_created = (
                time_diff / SECONDS_IN_MINUTE / MINUTES_IN_HOUR / HOURS_IN_DAY
            )
            subscriptions = stripe_client.get_subscriptions(stripe_customer_id)

            if days_since_account_created >= 7 and (not stripe_customer_id or not subscriptions):
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
