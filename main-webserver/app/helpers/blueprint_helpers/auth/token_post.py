from app import *
from app.helpers.utils.general.logs import *


def validateTokenHelper(token):
    if token:
        payload = jwt.decode(token, os.getenv("SECRET_KEY"))
        if payload["exp"] < dt.now().replace(tzinfo=timezone.utc).timestamp():
            return (
                jsonify({"status": UNAUTHORIZED, "error": "Expired token"}),
                UNAUTHORIZED,
            )
        else:
            return jsonify({"status": SUCCESS}), SUCCESS
    else:
        return jsonify({"status": UNAUTHORIZED, "error": "Invalid token"}), UNAUTHORIZED
