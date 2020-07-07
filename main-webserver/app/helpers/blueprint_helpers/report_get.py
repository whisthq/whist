from app import *
from app.helpers.utils.general.analytics import *
from app.helpers.utils.general.logs import *
from app.helpers.utils.general.sql_commands import *

def latestHelper():
    command = text(
        """
        SELECT *
        FROM status_report
        ORDER BY timestamp DESC
        """
    )
    output = fractalRunSQL(command, {})

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


    dayMins = totalMinutes(dayReport["rows"]) if dayReport["rows"] else 0
    weekMins = totalMinutes(weekReport["rows"]) if weekReport["rows"] else 0
    monthMins = totalMinutes(monthReport["rows"]) if monthReport["rows"] else 0

    return {"day": dayMins, "week": weekMins, "month": monthMins}


def signupsHelper():
    today = dt.now()
    command = text(
        """
            SELECT COUNT(created)
            FROM users
            WHERE created > :timestamp
        """
    )
    # try:
    #     with engine.connect() as conn:
    #         params = {
    #             "timestamp": dt.combine(today.date(), dt.min.time()).timestamp(),
    #         }
    #         dayCount = cleanFetchedSQL(conn.execute(command, **params).fetchall())[
    #             0
    #         ]["count"]
    #         params = {
    #             "timestamp": (today - datetime.timedelta(days=7)).timestamp(),
    #         }
    #         weekCount = cleanFetchedSQL(conn.execute(command, **params).fetchall())[
    #             0
    #         ]["count"]
    #         params = {
    #             "timestamp": (today - datetime.timedelta(days=30)).timestamp()
    #         }
    #         monthCount = cleanFetchedSQL(
    #             conn.execute(command, **params).fetchall()
    #         )[0]["count"]
    #         return (
    #             jsonify({"day": dayCount, "week": weekCount, "month": monthCount}),
    #             200,
    #         )
