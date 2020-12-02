from flask import current_app

from app.helpers.utils.general.tokens import getAccessTokens


def adminLoginHelper(username, password):
    if (
        username == current_app.config["DASHBOARD_USERNAME"]
        and password == current_app.config["DASHBOARD_PASSWORD"]
    ):
        access_token, refresh_token = getAccessTokens(username)

        return {"access_token": access_token, "refresh_token": refresh_token}
    else:
        return None
