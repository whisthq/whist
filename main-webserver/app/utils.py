from .imports import *


def genHaiku(n):
    """Generates an array of haiku names (no more than 15 characters) using haikunator

    Args:
        n (int): Length of the array to generate

    Returns:
        arr: An array of haikus
    """
    haikunator = Haikunator()
    haikus = [
        haikunator.haikunate(delimiter="") + str(np.random.randint(0, 10000))
        for _ in range(0, n)
    ]
    haikus = [haiku[0 : np.min([15, len(haiku)])] for haiku in haikus]
    return haikus


def generatePassword():
    upperCase = string.ascii_uppercase
    lowerCase = string.ascii_lowercase
    specialChars = "!@#$%^*"
    numbers = "1234567890"
    c1 = "".join([random.choice(upperCase) for _ in range(0, 3)])
    c2 = "".join([random.choice(lowerCase) for _ in range(0, 9)]) + c1
    c3 = "".join([random.choice(lowerCase) for _ in range(0, 5)]) + c2
    c4 = "".join([random.choice(numbers) for _ in range(0, 4)]) + c3
    return "".join(random.sample(c4, len(c4)))


def generateCode():
    upperCase = string.ascii_uppercase
    numbers = "1234567890"
    c1 = "".join([random.choice(numbers) for _ in range(0, 3)])
    c2 = "".join([random.choice(upperCase) for _ in range(0, 3)]) + "-" + c1
    return c2


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


def shiftUnixByDay(utc, num_days):
    date = unixToDate(utc)
    return round(dateToUnix(date + relativedelta(days=num_days)))


def shiftUnixByMonth(utc, num_months):
    date = unixToDate(utc)
    return round(dateToUnix(date + relativedelta(months=num_months)))


def shiftUnixByWeek(utc, num_weeks):
    date = unixToDate(utc)
    return round(dateToUnix(date + relativedelta(weeks=num_weeks)))


def shiftUnixByMinutes(utc, num_minutes):
    date = unixToDate(utc)
    return round(dateToUnix(date + relativedelta(minutes=num_minutes)))


def generateToken(user):
    token = jwt.encode({"email": user}, os.getenv("SECRET_KEY"))
    if len(token) > 15:
        token = token[-15:]
    else:
        token = token[-1 * len(pwd_token) :]

    return token


def cleanFetchedSQL(out):
    if out:
        is_list = isinstance(out, list)
        if is_list:
            return [dict(row) for row in out]
        else:
            return dict(out)
    return None


def getAccessTokens(user):
    # access_token = create_access_token(identity = user, expires_delta = datetime.timedelta(seconds=5))
    access_token = create_access_token(identity=user, expires_delta=False)
    refresh_token = create_refresh_token(identity=user, expires_delta=False)
    return (access_token, refresh_token)


def getGoogleTokens(code, clientApp):
    if clientApp:
        client_secret = "google_client_secret_desktop.json"
        redirect_uri = "urn:ietf:wg:oauth:2.0:oob:auto"
    else:
        client_secret = "google_client_secret.json"
        redirect_uri = "postmessage"

    flow = Flow.from_client_secrets_file(
        client_secret,
        scopes=[
            "https://www.googleapis.com/auth/userinfo.email",
            "openid",
            "https://www.googleapis.com/auth/userinfo.profile",
        ],
        redirect_uri=redirect_uri,
    )

    flow.fetch_token(code=code)

    credentials = flow.credentials

    session = flow.authorized_session()
    profile = session.get("https://www.googleapis.com/userinfo/v2/me").json()

    return {
        "access_token": credentials.token,
        "refresh_token": credentials.refresh_token,
        "google_id": profile["id"],
        "email": profile["email"],
        "name": profile["given_name"],
    }


def yieldNumber():
    num = 0
    while True:
        if num > 9999:
            num = 0
        num += 1
        yield str(num)
