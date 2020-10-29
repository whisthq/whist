from datetime import datetime as dt

from flask import current_app, jsonify
from jose import jwt

from app.constants.http_codes import SUCCESS, UNAUTHORIZED


def validateTokenHelper(token):
    if token:
        try:
            payload = jwt.decode(token, current_app.config["JWT_SECRET_KEY"])
        except Exception as e:
            return (jsonify({"status": UNAUTHORIZED, "error": "Expired token"}), UNAUTHORIZED)
        if payload["exp"] < dt.utcnow().timestamp():
            return (jsonify({"status": UNAUTHORIZED, "error": "Expired token"}), UNAUTHORIZED)
        else:
            return jsonify({"status": SUCCESS}), SUCCESS
    else:
        return jsonify({"status": UNAUTHORIZED, "error": "Invalid token"}), UNAUTHORIZED
