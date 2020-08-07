from app import *
from app.helpers.utils.general.logs import *
from app.helpers.utils.general.sql_commands import *
from app.helpers.utils.general.tokens import *


def adminLoginHelper(username, password):
    if username == getEnvVar("DASHBOARD_USERNAME") and password == getEnvVar(
        "DASHBOARD_PASSWORD"
    ):
        access_token, refresh_token = getAccessTokens(username)

        return {"access_token": access_token, "refresh_token": refresh_token}
    else:
        return None
