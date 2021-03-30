from typing import Dict, Optional

from flask import current_app

from app.helpers.utils.general.tokens import get_access_tokens


def admin_login_helper(username: str, password: str) -> Optional[Dict[str, str]]:
    if (
        username == current_app.config["DASHBOARD_USERNAME"]
        and password == current_app.config["DASHBOARD_PASSWORD"]
    ):
        access_token, refresh_token = get_access_tokens(username)

        return {"access_token": access_token, "refresh_token": refresh_token}
    else:
        return None
