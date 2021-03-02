import json
import datetime


from datetime import datetime as dt

from functools import wraps

from flask import current_app, jsonify, request
from flask_jwt_extended import get_jwt_identity

from app.models import User

from app.constants.http_codes import UNAUTHORIZED, PAYMENT_REQUIRED
from app.constants.time import SECONDS_IN_MINUTE, MINUTES_IN_HOUR, HOURS_IN_DAY
from app.helpers.utils.general.logs import fractal_logger
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
            fractal_logger.error(
                "Bearer error: {error}".format(error=str(e)),
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


def admin_required(func):
    """A decorator that will return unauthorized if the
    invoker is no the admin user (i.e. "DASHBOARD_USERNAME" user). This user can
    be found in the config db. To log in as this user you will have to use the admin
    login endpoint (which is different from the reguar login endpoint).

    Args:
        func (function): A function that will be wrapped by
        this decorator and only run if an admin is invoking it.

    Returns:
        function: A wrapper that will return unauthorized if the
        jwt identity provided is not an admin and otherwise run the wrapped function.
    """

    @wraps(func)
    def wrapper(*args, **kwargs):
        current_user = get_jwt_identity()

        if current_app.config["DASHBOARD_USERNAME"] not in current_user:
            return (
                jsonify(
                    {
                        "error": (
                            "Authorization failed. Provided username does not match"
                            "the username associated with the provided Bearer token."
                        ),
                    }
                ),
                UNAUTHORIZED,
            )

        return func(*args, **kwargs)

    return wrapper


def check_developer() -> bool:
    """Determine whether or not the sender of the current request is a Fractal developer.

    This function must be called with request context. More specifically, it should be called from
    within a view function that is decorated with @jwt_required().

    Returns:
        True iff the requester is a Fractal developer or the admin user.

    Raises:
        RuntimeError: This function was called before @jwt_required() and verify_jwt_in_request().
    """

    current_user = get_jwt_identity()

    return current_user is not None and (
        current_app.config["DASHBOARD_USERNAME"] in current_user
        or current_user.endswith("@fractal.co")
    )


def developer_required(func):
    """Similar decorator to admin_required, but it allows @fractal.co
    users as well. It is meant for developer endpoints such as the test_endpoint where we,
    for ease of testing, prefer to let any of the fractal members connect. It will return
    unauthorized for non-developers.

    Args:
        func (function): The function that is being decorated.

    Returns:
        function: A wrapper that runs some preprocessing/decoration code and
        then calls the func in certain cases (or returns something else). Check the
        docs on python decorators for more context.
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

        # admin/developer override
        if not check_developer():

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
