from app import *
from app.helpers.utils.general.analytics import *
from app.helpers.utils.general.logs import *

from app.models.hardware import *
from app.models.public import *

def latestHelper():
    log = MonitorLog.query.order_by(MonitorLog.timestamp).first()
    return log


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

    dayParams = dt.combine(today.date(), dt.min.time()).strftime("%m-%d-%y")
    dayReport = LoginHistory.query.filter(LoginHistory.timestamp > dayParams).order_by(LoginHistory.timestamp).all()

    weekParams = (today - datetime.timedelta(days=7)).strftime("%m-%d-%y")
    weekReport = LoginHistory.query.filter(LoginHistory.timestamp > weekParams).order_by(LoginHistory.timestamp).all()

    monthParams = (today - datetime.timedelta(days=30)).strftime("%m-%d-%y")
    monthReport = LoginHistory.query.filter(LoginHistory.timestamp > monthParams).order_by(LoginHistory.timestamp).all()

    dayMins = totalMinutes(dayReport) if dayReport else 0
    weekMins = totalMinutes(weekReport) if weekReport else 0
    monthMins = totalMinutes(monthReport) if monthReport else 0

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

    dayParams = dt.combine(today.date(), dt.min.time()).timestamp()
    dayCount = User.query.filter(User.created_timestamp > dayParams).count()

    weekParams = (today - datetime.timedelta(days=7)).timestamp(),
    weekCount = User.query.filter(User.created_timestamp > weekParams).count()

    monthParams = (today - datetime.timedelta(days=30)).timestamp()
    monthCount = User.query.filter(User.created_timestamp > monthParams).count()

    return {"day": dayCount, "week": weekCount, "month": monthCount}


def loginActivityHelper():
    activities = LoginHistory.query.order_by(LoginHistory.timestamp.desc()).all()
    return activities


def fetchUsersHelper():
    users = User.query.all()
    return users

def fetchVMsHelper():
    vms = UserVM.query.all()
    return vms

def fetchDisksHelper():
    disks = OSDisk.query.all()
    return disks
