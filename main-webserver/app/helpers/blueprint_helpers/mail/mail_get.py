from app import *
from app.helpers.utils.general.logs import *
from app.helpers.utils.general.sql_commands import *


def checkUserSubscribed(username):
    resp = fractalSQLSelect(table_name="newsletter", params={"username": username})[
        "rows"
    ]
    return jsonify({"subscribed": resp is not None and len(resp) > 0}), SUCCESS
