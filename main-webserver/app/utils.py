from .imports import *

def genHaiku(n):
    haikunator = Haikunator()
    haikus = [haikunator.haikunate(delimiter='') + str(np.random.randint(0, 10000)) for _ in range(0, n)]
    haikus = [haiku[0: np.min([15, len(haiku)])] for haiku in haikus]
    return haikus

def generatePassword():
    upperCase = string.ascii_uppercase
    lowerCase = string.ascii_lowercase
    specialChars = '!@#$%^*'
    numbers = '1234567890'
    c1 = ''.join([random.choice(upperCase) for _ in range(0,3)])
    c2 = ''.join([random.choice(lowerCase) for _ in range(0,9)]) + c1
    c3 = ''.join([random.choice(lowerCase) for _ in range(0,5)]) + c2
    c4 = ''.join([random.choice(numbers) for _ in range(0,4)]) + c3
    return ''.join(random.sample(c4,len(c4)))

def generateCode():
    upperCase = string.ascii_uppercase
    numbers = '1234567890'
    c1 = ''.join([random.choice(numbers) for _ in range(0,3)])
    c2 = ''.join([random.choice(upperCase) for _ in range(0,3)]) + '-' + c1
    return c2

def unixToDate(utc):
    return datetime.datetime.fromtimestamp(utc)

def dateToUnix(date):
    return round(date.timestamp())

def getToday():
    return datetime.datetime.now()

def shiftUnixByMonth(utc, num_months):
    date = unixToDate(utc)
    return round(dateToUnix(date + relativedelta(months=num_months)))

def shiftUnixByWeek(utc, num_weeks):
    date = unixToDate(utc)
    return round(dateToUnix(date + relativedelta(weeks=num_weeks)))

def generateToken(user):
    token = jwt.encode({'email': user}, os.getenv('SECRET_KEY'))
    if len(token) > 15:
        token = token[-15:]
    else:
        token = token[-1 * len(pwd_token):]

    return token

def getAccessTokens(user):
    access_token = create_access_token(identity = user)
    refresh_token = create_refresh_token(identity = user)
    return (access_token, refresh_token)
