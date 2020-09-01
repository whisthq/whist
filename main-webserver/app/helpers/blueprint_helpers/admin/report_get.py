from app import *
from app.helpers.utils.general.analytics import *
from app.helpers.utils.general.logs import *

from app.models.hardware import *
from app.models.public import *
from app.models.logs import *

from app.serializers.hardware import *
from app.serializers.public import *
from app.serializers.logs import *

monitor_log_schema = MonitorLogSchema()
login_history_schema = LoginHistorySchema()
user_schema = UserSchema()
vm_schema = UserVMSchema()
disk_schema = OSDiskSchema()

def latestHelper():
    log = MonitorLog.query.order_by(MonitorLog.timestamp).first()
    log = monitor_log_schema.dump(log)

    return log


def totalUsageHelper():
    today = dt.now()

    dayParams = dt.combine(today.date(), dt.min.time()).timestamp()
    dayReport = LoginHistory.query.filter(LoginHistory.timestamp > dayParams).order_by(LoginHistory.timestamp).all()

    weekParams = (today - datetime.timedelta(days=7)).timestamp()
    weekReport = LoginHistory.query.filter(LoginHistory.timestamp > weekParams).order_by(LoginHistory.timestamp).all()

    monthParams = (today - datetime.timedelta(days=30)).timestamp()
    monthReport = LoginHistory.query.filter(LoginHistory.timestamp > monthParams).order_by(LoginHistory.timestamp).all()

    dayMins = totalMinutes(dayReport) if dayReport else 0
    weekMins = totalMinutes(weekReport) if weekReport else 0
    monthMins = totalMinutes(monthReport) if monthReport else 0

    return {"day": dayMins, "week": weekMins, "month": monthMins}


def signupsHelper():
    today = dt.now()

    dayParams = dt.combine(today.date(), dt.min.time()).timestamp()
    dayCount = User.query.filter(User.created_timestamp > dayParams).count()

    weekParams = (today - datetime.timedelta(days=7)).timestamp(),
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

def fetchVMsHelper():
    vms = UserVM.query.all()
    vms = [vm_schema.dump(vm) for vm in vms]
    return vms

def fetchDisksHelper():
    disks = OSDisk.query.all()
    disks = [disk_schema.dump(disk) for disk in disks]
    return disks
