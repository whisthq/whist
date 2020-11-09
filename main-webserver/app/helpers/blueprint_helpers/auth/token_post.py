from flask import jsonify
from flask_jwt_extended import decode_token

from app.constants.http_codes import SUCCESS, UNAUTHORIZED


def validateTokenHelper(token):
    if token:
        try:
            payload = decode_token(token)
            email = payload["identity"]
        except Exception:
            return (jsonify({"status": UNAUTHORIZED, "error": "Expired token"}), UNAUTHORIZED)
        # if the token is expired decode_token throws an exception
        return jsonify({"status": SUCCESS, "user": email}), SUCCESS
    else:
        return jsonify({"status": UNAUTHORIZED, "error": "Invalid token"}), UNAUTHORIZED
