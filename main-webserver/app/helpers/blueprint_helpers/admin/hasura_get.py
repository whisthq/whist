from flask import current_app
from jose import jwt

from app.models import User


def authHelper(token):
    if not token:
        return {"X-Hasura-Role": "anonymous", "X-Hasura-User-Id": ""}
    else:
        token = token.replace("Bearer ", "")
        current_user = ""

        try:
            decoded_key = jwt.decode(token, current_app.config["JWT_SECRET_KEY"])
            if decoded_key:
                current_user = decoded_key["identity"]
        except Exception:
            return {"X-Hasura-Role": "anonymous", "X-Hasura-User-Id": ""}

        user = None if not current_user else User.query.get(current_user)

        if user and current_user:
            return {"X-Hasura-Role": "user", "X-Hasura-User-Id": current_user}

        return {"X-Hasura-Role": "anonymous", "X-Hasura-User-Id": ""}
