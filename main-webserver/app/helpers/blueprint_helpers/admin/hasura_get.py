from flask_jwt_extended import get_jwt_identity
from jose import jwt

from app.constants.config import JWT_SECRET_KEY
from app.models import User

from app.helpers.utils.general.logs import fractalLog


def authHelper(token):
    if not token:
        return {"X-Hasura-Role": "anonymous", "X-Hasura-User-Id": ""}
    else:
        token = token.replace("Bearer ", "")
        current_user = ""

        try:
            decoded_key = jwt.decode(token, JWT_SECRET_KEY)
            if decoded_key:
                current_user = decoded_key["identity"]
        except Exception as e:
            return {"X-Hasura-Role": "anonymous", "X-Hasura-User-Id": ""}

        user = None if not current_user else User.query.get(current_user)

        if user and current_user:
            return {"X-Hasura-Role": "user", "X-Hasura-User-Id": current_user}

        return {"X-Hasura-Role": "anonymous", "X-Hasura-User-Id": ""}
