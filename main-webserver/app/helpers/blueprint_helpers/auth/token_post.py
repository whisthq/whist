from app import *
from app.helpers.utils.general.logs import *
from app.helpers.utils.general.sql_commands import *


def validateTokenHelper(token):
    params = {"token": token}
    token = fractalSQLSelect("password_tokens", params)["rows"]
    if token:
        timeIssued = token[0]["time_issued"]
        diff = (
            dt.now() - dt.strptime(timeIssued, "%m-%d-%Y, %H:%M:%S")
        ).total_seconds()
        if diff > (60 * 10):
            return (
                jsonify({"status": UNAUTHORIZED, "error": "Expired token"}),
                UNAUTHORIZED,
            )
        else:
            return jsonify({"status": SUCCESS}), SUCCESS
    else:
        return jsonify({"status": UNAUTHORIZED, "error": "Invalid token"}), UNAUTHORIZED
