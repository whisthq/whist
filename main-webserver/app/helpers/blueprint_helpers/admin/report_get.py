from app import *
from app.helpers.utils.general.analytics import *
from app.helpers.utils.general.logs import *
from app.helpers.utils.general.sql_commands import *


def latestHelper():
    session = Session()

    rows = (
        session.query(MonitorLog).order_by(MonitorLog.timestamp)
    )

    session.commit()
    session.close()

    rows = rows.all()
    output = []
    for row in rows:
        row.__dict__.pop("_sa_instance_state", None)
        output.append(row.__dict__)


    if output:
        return output[0]
    else:
        return None


def totalUsageHelper():
    session = Session()

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
        "date": dt.combine(today.date(), dt.min.time()).strftime("%m-%d-%y"),
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

    dayParams = {
        "timestamp": dt.combine(today.date(), dt.min.time()).timestamp(),
    }
    dayOutput = fractalRunSQL(command, dayParams)
    dayCount = dayOutput["rows"][0]["count"] if dayOutput["rows"] else 0

    weekParams = {
        "timestamp": (today - datetime.timedelta(days=7)).timestamp(),
    }
    weekOutput = fractalRunSQL(command, weekParams)
    weekCount = weekOutput["rows"][0]["count"] if weekOutput["rows"] else 0

    monthParams = {"timestamp": (today - datetime.timedelta(days=30)).timestamp()}
    monthOutput = fractalRunSQL(command, monthParams)
    monthCount = monthOutput["rows"][0]["count"] if monthOutput["rows"] else 0

    return {"day": dayCount, "week": weekCount, "month": monthCount}


def loginActivityHelper():
    output = fractalSQLSelect("login_history", {})
    if output["rows"]:
        activities = output["rows"]
        activities.reverse()
        return activities
    else:
        return {}


def fetchUsersHelper():
    output = fractalSQLSelect("users", {})
    if output["rows"]:
        return output["rows"]
    else:
        return {}


def fetchVMsHelper():
    output = fractalSQLSelect("v_ms", {})
    if output["rows"]:
        return output["rows"]
    else:
        return {}


def fetchCustomersHelper():
    output = fractalSQLSelect("customers", {})
    if output["rows"]:
        return output["rows"]
    else:
        return {}


def fetchDisksHelper():
    # TODO: delete branch from disks and fix this command
    command = text(
        """
        SELECT disks.*, disk_settings.using_stun, disk_settings.branch as settings_branch
        FROM disks
        LEFT JOIN disk_settings ON disks.disk_name = disk_settings.disk_name
        """
    )
    output = fractalRunSQL(command, {})
    if output["rows"]:
        return output["rows"]
    else:
        return {}
