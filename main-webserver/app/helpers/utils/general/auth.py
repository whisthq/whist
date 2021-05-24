from functools import wraps

from flask_jwt_extended import get_jwt, verify_jwt_in_request

from _stripe import ensure_subscribed
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
        if not has_scope("admin"):
            verify_jwt_in_request()
            ensure_subscribed(get_jwt()["https://api.fractal.co/stripe_customer_id"])

        return func(*args, **kwargs)

    return wrapper
