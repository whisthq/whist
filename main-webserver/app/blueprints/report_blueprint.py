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
    elif action == "userReport" and request.method == "POST":
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
    return jsonify({}), 404
