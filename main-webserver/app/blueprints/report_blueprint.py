from app import *

from app.logger import *

report_bp = Blueprint("report_bp", __name__)


@report_bp.route("/report/<action>", methods=["GET", "POST"])
def statusReport(action, **kwargs):
    if action == "latest" and request.method == "GET":
        command = text(
            """
        SELECT * 
        FROM status_report
        ORDER BY timestamp DESC
        """
        )
        params = {}
        try:
            with engine.connect() as conn:
                latest = cleanFetchedSQL(conn.execute(command, **params).fetchone())
                return jsonify(latest), 200
        except:
            traceback.print_exc()
    elif action == "regionReport" and request.method == "POST":
        body = request.get_json()
        command = text("")
        if body["timescale"] == "day":
            command = text(
                """
            SELECT timestamp, users_online, eastus_unavailable, northcentralus_unavailable, southcentralus_unavailable
            FROM status_report
            ORDER BY timestamp DESC
            LIMIT 24
            """
            )
        elif body["timescale"] == "week":
            command = text(
                """
            SELECT date_trunc('day', to_timestamp("timestamp")) as "timestamp", SUM(users_online) as "users_online", SUM(eastus_unavailable) as "eastus_unavailable", SUM(northcentralus_unavailable) as "northcentralus_unavailable", SUM(southcentralus_unavailable) as "southcentralus_unavailable"
            FROM status_report
            GROUP BY 1
            ORDER BY date_trunc('day', to_timestamp("timestamp")) DESC
            LIMIT 7
            """
            )
        elif body["timescale"] == "month":
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
        try:
            with engine.connect() as conn:
                report = cleanFetchedSQL(conn.execute(command, **params).fetchall())
                if body["timescale"] == "week" or body["timescale"] == "month":
                    for row in report:
                        row["timestamp"] = int(row["timestamp"].timestamp())

                return jsonify(report), 200
        except:
            traceback.print_exc()
    elif action == "userReport" and request.method == "POST":
        body = request.get_json()
        today = datetime.date.today()
        command = text("")
        params = {}
        if body["timescale"] == "day":
            command = text(
                """
            """
            )
        elif body["timescale"] == "week":
            lastWeek = today - datetime.timedelta(days=7)
            command = text(
                """
            SELECT *
            FROM login_history
            WHERE "username" = :username AND timestamp > :date AND is_user = true
            ORDER BY timestamp ASC
            """
            )
            params = {"username": body["username"], "date": lastWeek.strftime("%m-%d-%y")}
        elif body["timescale"] == "month":
            lastMonth = today - datetime.timedelta(days=30)
            command = text(
                """
            SELECT *
            FROM login_history
            WHERE "username" = :username AND timestamp > :date AND is_user = true
            ORDER BY timestamp ASC
            """
            )
            params = {"username": body["username"], "date": lastMonth.strftime("%m-%d-%y")}
        try:
            with engine.connect() as conn:
                report = cleanFetchedSQL(conn.execute(command, **params).fetchall())
                output = []
                if body["timescale"] == "week" or body["timescale"] == "month":
                    output = loginsToMinutes(report)

                return jsonify(output), 200
        except:
            traceback.print_exc()
            return jsonify({}), 500
    return jsonify({}), 404


def loginsToMinutes(report):
    """Turns an array of login history to an array of time online per day

    Args:
        report (arr[dict]): Report entries from login_history sql table, MUST be sorted in ascending order

    Returns:
        arr[dict]: The output array
    """
    index = 1
    output = []
    minutesOnline = 0

    while index < len(report):
        earlyTime = dt.strptime(report[index - 1]["timestamp"], "%m-%d-%Y, %H:%M:%S")
        lateTime = dt.strptime(report[index]["timestamp"], "%m-%d-%Y, %H:%M:%S")

        if earlyTime.date() == lateTime.date():  # Same day
            if report[index]["action"] != report[index - 1]["action"]:
                if report[index]["action"] == "logoff":
                    deltaTime = lateTime - earlyTime
                    minutesOnline += deltaTime.seconds / 60
            else:
                if report[index]["action"] == "logon":
                    deltaTime = lateTime - earlyTime
                    minutesOnline += deltaTime.seconds / 60
        else:
            if (
                report[index - 1]["action"] == "logon"
            ):  # User session continued to next day
                midnight = dt.combine(lateTime.date(), dt.min.time())
                deltaTime = midnight - earlyTime
                minutesOnline += deltaTime.seconds / 60
            if minutesOnline > 1:
                output.append(
                    {
                        "timestamp": int(dt.timestamp(earlyTime)),
                        "minutes": int(minutesOnline),
                    }
                )
            minutesOnline = 0
            if (
                report[index - 1]["action"] == "logon"
            ):  # User session continued to next day
                midnight = dt.combine(lateTime.date(), dt.min.time())
                deltaTime = lateTime - midnight
                minutesOnline += deltaTime.seconds / 60

        if index == len(report) - 1:  # Last entry
            if report[index]["action"] == "logon":
                deltaTime = datetime.date.today() - lateTime
                minutesOnline += deltaTime.seconds / 60
            if minutesOnline > 1:
                output.append(
                    {
                        "timestamp": int(dt.timestamp(earlyTime)),
                        "minutes": int(minutesOnline),
                    }
                )
        index += 1
    return output
