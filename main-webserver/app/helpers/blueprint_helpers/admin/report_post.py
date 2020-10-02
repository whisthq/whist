from app import *
from app.helpers.utils.general.analytics import *
from app.helpers.utils.general.logs import *
from app.helpers.utils.general.sql_commands import *

from app.models.logs import *
from app.serializers.logs import *

# this is being @deprecated
def regionReportHelper(timescale):
    """
    Queries from when we directly queried:
    (note that the actual implementation yields some extra columns; this is fine)

    SELECT timestamp, users_online, eastus_unavailable, northcentralus_unavailable, southcentralus_unavailable
    FROM logs.monitor_logs
    ORDER BY timestamp DESC
    LIMIT 24

    SELECT date_trunc('day', to_timestamp("timestamp")) as "timestamp", SUM(users_online) as "users_online", SUM(eastus_unavailable) as "eastus_unavailable", SUM(northcentralus_unavailable) as "northcentralus_unavailable", SUM(southcentralus_unavailable) as "southcentralus_unavailable"
    FROM logs.monitor_logs
    GROUP BY 1
    ORDER BY date_trunc('day', to_timestamp("timestamp")) DESC
    LIMIT 7

    SELECT date_trunc('day', to_timestamp("timestamp")) as "timestamp", SUM(users_online) as "users_online", SUM(eastus_unavailable) as "eastus_unavailable", SUM(northcentralus_unavailable) as "northcentralus_unavailable", SUM(southcentralus_unavailable) as "southcentralus_unavailable"
    FROM logs.monitor_logs
    GROUP BY 1
    ORDER BY date_trunc('day', to_timestamp("timestamp")) DESC
    LIMIT 30
    """
    monitor_log_schema = MonitorLogSchema()

    # TODO (adriano) not urgent; works, but not exactly the original
    # did it this way because you can't construct a query since it's like already a BaseQuery or something
    output = None
    if timescale == "day":
        output = MonitorLog.query.order_by(MonitorLog.timestamp.desc()).limit(24).all()
    elif timescale == "week":
        output = MonitorLog.query.order_by(MonitorLog.timestamp.desc()).limit(24 * 7).all()
    elif timescale == "month":
        output = MonitorLog.query.order_by(MonitorLog.timestamp.desc()).limit(24 * 7 * 30).all()

    output = [monitor_log_schema.dump(element) for element in output]

    return output


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
                "{year}-{month}-{day}".format(year=today.year, month=today.month, day="1"),
                "%Y-%m-%d",
            )

            date = int(dt.timestamp(beginning_month))

    if start_date:
        date = start_date

    user = User.query.get(username)
    if not user:
        # FIXME somehow we keep getting here
        return jsonify({"error": "user with email does not exist!"}), BAD_REQUEST

    histories = (
        LoginHistory.query.filter(
            (LoginHistory.user_id == username) & (LoginHistory.timestamp > date)
        )
        .order_by(LoginHistory.timestamp)
        .all()
    )

    # import and use serializers
    histories = [login_history_schema.dump(history) for history in histories]

    output = loginsToMinutes(histories)

    return jsonify(output), SUCCESS
