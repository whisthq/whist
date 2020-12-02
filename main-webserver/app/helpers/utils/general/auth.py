import json
import logging

from functools import wraps

from flask import current_app, jsonify, request
from flask_jwt_extended import get_jwt_identity

from app.constants.http_codes import UNAUTHORIZED
from app.helpers.utils.general.logs import fractalLog


def fractalAuth(f):
    @wraps(f)
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
            fractalLog(
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

        return f(*args, **kwargs)

    return wrapper


def adminRequired(f):
    @wraps(f)
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

        return f(*args, **kwargs)

    return wrapper
