import datetime

from datetime import datetime as dt

from app.helpers.utils.general.analytics import totalMinutes
from app.models.logs import LoginHistory, MonitorLog
from app.models.public import User
from app.serializers.logs import LoginHistorySchema, MonitorLogSchema
from app.serializers.public import UserSchema

monitor_log_schema = MonitorLogSchema()
login_history_schema = LoginHistorySchema()
user_schema = UserSchema()


def latestHelper():
    log = MonitorLog.query.order_by(MonitorLog.timestamp).first()
    log = monitor_log_schema.dump(log)

    return log


def totalUsageHelper():
    today = dt.now()

    dayParams = dt.combine(today.date(), dt.min.time()).timestamp()
    dayReport = login_history_schema.dump(
        LoginHistory.query.filter(LoginHistory.timestamp > dayParams)
        .order_by(LoginHistory.timestamp)
        .all()
    )

    weekParams = (today - datetime.timedelta(days=7)).timestamp()
    weekReport = login_history_schema.dump(
        LoginHistory.query.filter(LoginHistory.timestamp > weekParams)
        .order_by(LoginHistory.timestamp)
        .all()
    )

    monthParams = (today - datetime.timedelta(days=30)).timestamp()
    monthReport = login_history_schema.dump(
        LoginHistory.query.filter(LoginHistory.timestamp > monthParams)
        .order_by(LoginHistory.timestamp)
        .all()
    )

    # FIXME why is this empty?!? this doesn't work

    dayMins = totalMinutes(dayReport) if dayReport else 0
    weekMins = totalMinutes(weekReport) if weekReport else 0
    monthMins = totalMinutes(monthReport) if monthReport else 0

    return {"day": dayMins, "week": weekMins, "month": monthMins}


# FIXME
def signupsHelper():
    today = dt.now()

    dayParams = dt.combine(today.date(), dt.min.time()).timestamp()
    dayCount = User.query.filter(User.created_timestamp > dayParams).count()

    weekParams = ((today - datetime.timedelta(days=7)).timestamp(),)
    weekCount = User.query.filter(User.created_timestamp > weekParams).count()

    monthParams = (today - datetime.timedelta(days=30)).timestamp()
    monthCount = User.query.filter(User.created_timestamp > monthParams).count()

    return {"day": dayCount, "week": weekCount, "month": monthCount}


def loginActivityHelper():
    activities = LoginHistory.query.order_by(LoginHistory.timestamp.desc()).all()
    activities = [login_history_schema.dump(activity) for activity in activities]
    return activities


def fetchUsersHelper():
    users = User.query.all()
    users = [user_schema.dump(user) for user in users]
    return users
