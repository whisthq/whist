from flask_jwt_extended import get_jwt_identity

from app.models import User


def authHelper():
    current_user = get_jwt_identity()

    user = User.query.get(current_user)

    if user:
        return {"X-Hasura-Role": "user", "X-Hasura-User-Id": current_user}
    else:
        return {"X-Hasura-Role": "anonymous", "X-Hasura-User-Id": None}
