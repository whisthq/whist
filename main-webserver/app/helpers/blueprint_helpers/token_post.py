from app import *
from app.helpers.utils.general.logs import *
from app.helpers.utils.general.sql_commands import *


def validateTokenHelper(token):
    params = {"token": token}
    user = fractalSQLSelect("password_tokens", params)
    if user:
        timeIssued = user[0][1]
        diff = (
            dt.now() - dt.strptime(timeIssued, "%m-%d-%Y, %H:%M:%S")
        ).total_seconds()
        conn.close()
        if diff > (60 * 10):
            return (
                jsonify({"status": UNAUTHORIZED, "error": "Expired token"}),
                UNAUTHORIZED,
            )
        else:
            return jsonify({"status": SUCCESS}), SUCCESS
    else:
        conn.close()
        return jsonify({"status": UNAUTHORIZED, "error": "Invalid token"}), UNAUTHORIZED
