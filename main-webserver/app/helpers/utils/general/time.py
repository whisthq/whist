import datetime


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
