from datetime import datetime as dt

from flask import current_app, jsonify
from jose import jwt

from app.constants.http_codes import SUCCESS, UNAUTHORIZED
from app.helpers.utils.general.logs import fractalLog

def validateTokenHelper(token):
    if token:
        fractalLog("TOKEN IS", "TOKEN IS", str(token))
        try:
            payload = jwt.decode(token, current_app.config["JWT_SECRET_KEY"])
        except Exception as e:
            return (jsonify({"status": UNAUTHORIZED, "error": "Expired token"}), UNAUTHORIZED)
        fractalLog("PAYLOAD IS", "PAYLOAD IS", str(payload))
        if payload["exp"] < dt.utcnow().timestamp():
            return (jsonify({"status": UNAUTHORIZED, "error": "Expired token"}), UNAUTHORIZED)
        else:
            return jsonify({"status": SUCCESS, "user": payload['email']}), SUCCESS
    else:
        return jsonify({"status": UNAUTHORIZED, "error": "Invalid token"}), UNAUTHORIZED
