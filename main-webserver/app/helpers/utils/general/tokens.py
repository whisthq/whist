from flask_jwt_extended import create_access_token, create_refresh_token


def get_access_tokens(username):
    access_token = create_access_token(identity=username, expires_delta=False)
    refresh_token = create_refresh_token(identity=username, expires_delta=False)
    return (access_token, refresh_token)
