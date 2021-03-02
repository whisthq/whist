from flask_jwt_extended import get_jwt_identity
from app.helpers.utils.general.logs import fractal_log
from app.helpers.blueprint_helpers.auth.account_get import fetch_user_helper
import json


def validate_token_helper():

    current_user = get_jwt_identity()
    output = fetch_user_helper(current_user)
    fractal_log(
        function="validate_token_helper",
        label=None,
        logs="Current user: " + json.dumps(current_user),
    )
    return output
