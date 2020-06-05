from app import *

from app.logger import *

report_bp = Blueprint("report_bp", __name__)


@report_bp.route("/report/<action>", methods=["GET"])
def statusReport(action, **kwargs):
    if action == "latest":
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
    elif action == "userReport":
        command = text(
            """
        SELECT timestamp, users_online, eastus_unavailable, northcentralus_unavailable, southcentralus_unavailable
        FROM status_report
        ORDER BY timestamp ASC
        LIMIT 24
        """
        )
        params = {}
        try:
            with engine.connect() as conn:
                users = cleanFetchedSQL(conn.execute(command, **params).fetchall())
                return jsonify(users), 200
        except:
            traceback.print_exc()
    return jsonify({}), 404
