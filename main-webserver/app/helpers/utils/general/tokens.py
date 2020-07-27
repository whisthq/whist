from app.imports import *
from app.helpers.utils.general.sql_commands import *

def generatePrivateKey():
    return secrets.token_hex(16)

def generateUniquePromoCode():
    output = fractalSQLSelect("users", {})
    old_codes = []
    if output["rows"]:
        old_codes = [user["code"] for user in output["rows"]]
    new_code = generatePromoCode()
    while new_code in old_codes:
        new_code = generatePromoCode()
    return new_code

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


def getGoogleTokens(code, clientApp):
    if clientApp:
        client_secret = "secrets/google_client_secret_desktop.json"
        redirect_uri = "urn:ietf:wg:oauth:2.0:oob:auto"
    else:
        client_secret = "secrets/google_client_secret.json"
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
