import datetime

from dateutil.relativedelta import relativedelta


def unix_to_date(utc):
    return datetime.datetime.fromtimestamp(utc)


def date_to_string(time):
    return time.strftime("%m/%d/%Y, %H:%M")


def get_current_time():
    return date_to_string(datetime.datetime.now())


def date_to_unix(date):
    return round(date.timestamp())


def get_today():
    aware = datetime.datetime.now()
    return aware


def shift_unix_by_month(utc, num_months):
    date = unix_to_date(utc)
    return round(date_to_unix(date + relativedelta(months=num_months)))


def shift_unix_by_week(utc, num_weeks):
    date = unix_to_date(utc)
    return round(date_to_unix(date + relativedelta(weeks=num_weeks)))


def shift_unix_by_minutes(utc, num_minutes):
    date = unix_to_date(utc)
    return round(date_to_unix(date + relativedelta(minutes=num_minutes)))
