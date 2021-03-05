import secrets

from flask_jwt_extended import create_access_token, create_refresh_token


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
