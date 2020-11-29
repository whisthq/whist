from app.constants.config import DASHBOARD_PASSWORD, DASHBOARD_USERNAME
from app.helpers.utils.general.tokens import getAccessTokens


def adminLoginHelper(username, password):
    if username == DASHBOARD_USERNAME and password == DASHBOARD_PASSWORD:
        access_token, refresh_token = getAccessTokens(username)

        return {"access_token": access_token, "refresh_token": refresh_token}
    else:
        return None
