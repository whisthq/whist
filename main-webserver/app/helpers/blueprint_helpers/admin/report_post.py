from app import *
from app.helpers.utils.general.analytics import *
from app.helpers.utils.general.logs import *
from app.helpers.utils.general.sql_commands import *

from app.models.logs import *
from app.serializers.logs import *

def regionReportHelper(timescale):
    command = text("")
    if timescale == "day":
        command = text(
            """
        SELECT timestamp, users_online, eastus_unavailable, northcentralus_unavailable, southcentralus_unavailable
        FROM status_report
        ORDER BY timestamp DESC
        LIMIT 24
        """
        )
    elif timescale == "week":
        command = text(
            """
        SELECT date_trunc('day', to_timestamp("timestamp")) as "timestamp", SUM(users_online) as "users_online", SUM(eastus_unavailable) as "eastus_unavailable", SUM(northcentralus_unavailable) as "northcentralus_unavailable", SUM(southcentralus_unavailable) as "southcentralus_unavailable"
        FROM status_report
        GROUP BY 1
        ORDER BY date_trunc('day', to_timestamp("timestamp")) DESC
        LIMIT 7
        """
        )
    elif timescale == "month":
        command = text(
            """
        SELECT date_trunc('day', to_timestamp("timestamp")) as "timestamp", SUM(users_online) as "users_online", SUM(eastus_unavailable) as "eastus_unavailable", SUM(northcentralus_unavailable) as "northcentralus_unavailable", SUM(southcentralus_unavailable) as "southcentralus_unavailable"
        FROM status_report
        GROUP BY 1
        ORDER BY date_trunc('day', to_timestamp("timestamp")) DESC
        LIMIT 30
        """
        )
    params = {}

    output = fractalRunSQL(command, params)
    if output["rows"]:
        report = output["rows"]
        if timescale == "week" or timescale == "month":
            for row in report:
                row["timestamp"] = int(row["timestamp"].timestamp())
        return report
    else:
        return {}


def userReportHelper(username, timescale=None, start_date=None):
    login_history_schema = LoginHistorySchema()

    today = dt.now()

    date = 0

    if timescale:
        if timescale == "day":
            return []
        elif timescale == "week":
            lastWeek = today - datetime.timedelta(days=7)
            date = int(dt.timestamp(lastWeek))
        elif timescale == "month":
            lastMonth = today - datetime.timedelta(days=30)
            date = int(dt.timestamp(lastMonth))
        elif timescale == "beginningMonth":
            beginning_month = dt.strptime(
                "{year}-{month}-{day}".format(
                    year=today.year, month=today.month, day="1"
                ),
                "%Y-%m-%d",
            )

            date = int(dt.timestamp(beginning_month))

    if start_date:
        date = start_date

    user = User.query.filter_by(user_id=username).first()
    if not user:
        return jsonify({"error": "user with email does not exist!"}), BAD_REQUEST

    histories = LoginHistory.query.filter((LoginHistory.user_id == username) & (LoginHistory.timestamp > date)).order_by(LoginHistory.timestamp).all()

    # import and use serializers
    histories = [login_history_schema.dump(history) for history in histories]

    output = loginsToMinutes(histories)

    return jsonify(output), SUCCESS
