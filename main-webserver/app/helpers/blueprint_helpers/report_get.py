from app import *
from app.helpers.utils.general.analytics import *
from app.helpers.utils.general.logs import *
from app.helpers.utils.general.sql_commands import *

def latestHelper():
    command = text(
        """"
        SELECT *
        FROM status_report
        ORDER BY timestamp DESC
        """
    )
    output = fractalRunSQL(command)

    if output["success"] and output["rows"]:
        return output["rows"][0]
    else:
        return None

def totalUsageHelper():
    today = dt.now()
    command = text(
        """
        SELECT *
        FROM login_history
        WHERE timestamp > :date AND is_user = true
        ORDER BY timestamp ASC
        """
    )

    dayParams = {
        "date": dt.combine(today.date(), dt.min.time()).strftime(
            "%m-%d-%y"
        ),
    }
    dayReport = fractalRunSQL(command, dayParams)

    weekParams = {
        "date": (today - datetime.timedelta(days=7)).strftime("%m-%d-%y"),
    }
    weekReport = fractalRunSQL(command, weekParams)

    monthParams = {
        "date": (today - datetime.timedelta(days=30)).strftime("%m-%d-%y"),
    }
    monthReport = fractalRunSQL(command, monthParams)

    if dayReport and weekReport and monthReport:
        dayMins = totalMinutes(dayReport)
        weekMins = totalMinutes(weekReport)
        monthMins = totalMinutes(monthReport)

        return jsonify({"day": dayMins, "week": weekMins, "month": monthMins})
    else:
        return None
