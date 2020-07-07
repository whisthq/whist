from app import *
from app.helpers.utils.general.analytics import *
from app.helpers.utils.general.logs import *
from app.helpers.utils.general.sql_commands import *

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
