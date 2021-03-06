import hashlib
import random
import secrets
import string

from better_profanity import profanity
from flask_jwt_extended import create_access_token, create_refresh_token
from google_auth_oauthlib.flow import Flow

from app.constants.bad_words_hashed import BAD_WORDS_HASHED
from app.models import User


def generate_private_key():
    return secrets.token_hex(16)


def generate_unique_promo_code():
    users = User.query.all()
    old_codes = []
    if users:
        old_codes = [user.referral_code for user in users]
    new_code = generate_promo_code()
    while new_code in old_codes:
        new_code = generate_promo_code()
    return new_code


def get_access_tokens(username):
    access_token = create_access_token(identity=username, expires_delta=False)
    refresh_token = create_refresh_token(identity=username, expires_delta=False)
    return (access_token, refresh_token)


def generate_token(username):
    token = create_access_token(identity=username)
    if len(token) > 15:
        token = token[-15:]
    else:
        token = token[-1 * len(token) :]

    return token


def generate_promo_code():
    upper_case = string.ascii_uppercase
    numbers = "1234567890"

    allowed = False
    while not allowed:
        number_segment = "".join([random.choice(numbers) for _ in range(0, 3)])
        full_code = "".join([random.choice(upper_case) for _ in range(0, 3)]) + "-" + number_segment
        full_code_encoding = full_code.lower().encode("utf-8")
        if hashlib.md5(
            full_code_encoding
        ).hexdigest() not in BAD_WORDS_HASHED and not profanity.contains_profanity(full_code):
            allowed = True
    return full_code


def get_google_tokens(code, client_app):
    if client_app:
        client_secret = "secrets/google_client_secret_desktop.json"
        redirect_uri = "com.tryfractal.app:/oauth2Callback"
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
