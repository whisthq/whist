import json
import logging

from functools import wraps

from flask import current_app, jsonify, request
from flask_jwt_extended import get_jwt_identity

from app.constants.http_codes import UNAUTHORIZED
from app.helpers.utils.general.logs import fractal_log


def fractal_auth(func):
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


def developerAccess(f):
    # in the future we may want to instead make a table in the DB
    # for people who are developers
    # TODO (adriano) ^
    @wraps(f)
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

        return f(*args, **kwargs)

    return wrapper
