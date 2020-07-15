from app.imports import *


def unixToDate(utc):
    return datetime.datetime.fromtimestamp(utc)


def dateToString(time):
    return time.strftime("%m/%d/%Y, %H:%M")


def getCurrentTime():
    return dateToString(datetime.datetime.now())


def dateToUnix(date):
    return round(date.timestamp())


def getToday():
    aware = datetime.datetime.now()
    return aware


def shiftUnixByMonth(utc, num_months):
    date = unixToDate(utc)
    return round(dateToUnix(date + relativedelta(months=num_months)))


def shiftUnixByWeek(utc, num_weeks):
    date = unixToDate(utc)
    return round(dateToUnix(date + relativedelta(weeks=num_weeks)))


def shiftUnixByMinutes(utc, num_minutes):
    date = unixToDate(utc)
    return round(dateToUnix(date + relativedelta(minutes=num_minutes)))
