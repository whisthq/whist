import secrets

from flask_jwt_extended import create_access_token, create_refresh_token
from google_auth_oauthlib.flow import Flow


def generate_private_key():
    return secrets.token_hex(16)


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
