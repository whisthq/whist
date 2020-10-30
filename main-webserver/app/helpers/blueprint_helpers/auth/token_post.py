from datetime import datetime as dt

from flask import current_app, jsonify
from jose import jwt

from app.constants.http_codes import SUCCESS, UNAUTHORIZED
from app.helpers.utils.general.logs import fractalLog
from flask_jwt_extended import create_access_token
from datetime import timedelta

def validateTokenHelper(token):
    if token:
        fractalLog("TOKEN IS", "TOKEN IS", str(token))
        try:
            payload = jwt.decode(token, current_app.config["JWT_SECRET_KEY"])
            email = payload['email']
            modify_token = create_access_token(identity=email, expires_delta=timedelta(minutes=10))
        except Exception as e:
            return (jsonify({"status": UNAUTHORIZED, "error": "Expired token"}), UNAUTHORIZED)
        # if try succeded
        if payload["exp"] < dt.utcnow().timestamp():
            return (jsonify({"status": UNAUTHORIZED, "error": "Expired token"}), UNAUTHORIZED)
        else:
            # email and token are necessary to modify user on reset
            # in the future we'll want to avoid actions like this
            return jsonify({"status": SUCCESS, "user": email, "token": modify_token}), SUCCESS
    else:
        return jsonify({"status": UNAUTHORIZED, "error": "Invalid token"}), UNAUTHORIZED
