from app.imports import *


def getAccessTokens(username):
    # access_token = create_access_token(identity = username, expires_delta = datetime.timedelta(seconds=5))
    access_token = create_access_token(identity=username, expires_delta=False)
    refresh_token = create_refresh_token(identity=username, expires_delta=False)
    return (access_token, refresh_token)


def generateToken(username):
    token = jwt.encode({"email": username}, os.getenv("SECRET_KEY"))
    if len(token) > 15:
        token = token[-15:]
    else:
        token = token[-1 * len(pwd_token) :]

    return token


def generatePromoCode():
    upperCase = string.ascii_uppercase
    numbers = "1234567890"
    c1 = "".join([random.choice(numbers) for _ in range(0, 3)])
    c2 = "".join([random.choice(upperCase) for _ in range(0, 3)]) + "-" + c1
    return c2
