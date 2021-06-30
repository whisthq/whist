"""Utilities for enforcing scoped access.

Example usage:

    from auth0 import scope_required
    from flask import Flask

    app = Flask(__name__)

    app.config["AUTH0_DOMAIN"] = "auth.fractal.co"
    app.config["JWT_DECODE_ALGORITHMS"] = ("RS256",)
    app.config["JWT_DECODE_AUDIENCE"] = "https://api.fractal.co/"

    auth0 = Auth0(app)

    @app.route("/")
    @scope_required("admin")
    def hello():
        return "Hello world"

"""

# TRIVIAL COMMIT FOR CI

import functools

from typing import Callable

from flask_jwt_extended import get_jwt, verify_jwt_in_request


class ScopeError(Exception):
    """Raised when an access token does not include the scopes required to access a resource."""


def has_scope(scope: str) -> bool:
    """Determine whether or not an access token contains a claim to the specified scope.

    Args:
        scope: A string containing the name of an OAuth 2.0 scope.

    Returns:
        True if and only if the specified scope appears in the JWT's scope claim.
    """

    verify_jwt_in_request(optional=True)

    return scope in get_jwt().get("scope", "").split()


def scope_required(scope: str) -> Callable:
    """Allow a user to access a decorated endpoint if their access token has the proper scope.

    Instructions for writing custom authorization decorators can be found here:
    https://flask-jwt-extended.readthedocs.io/en/stable/custom_decorators/

    Args:
        scope: A string indicating the name of the OAuth 2.0 scope by which this view function is
            protected.

    Returns:
        A decorator that can be applied to a Flask view function to enforce authentication.
    """

    def view_decorator(view_func):
        @functools.wraps(view_func)
        def decorated_view(*args, **kwargs):
            verify_jwt_in_request()

            if not has_scope(scope):
                raise ScopeError("Insufficient permissions")

            return view_func(*args, **kwargs)

        return decorated_view

    return view_decorator
