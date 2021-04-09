from typing import Dict, Union, Optional

from app.constants.http_codes import BAD_REQUEST, SUCCESS
from app.models import User
from app.serializers.public import UserSchema
from app.helpers.utils.general.tokens import get_access_tokens

user_schema = UserSchema()


def fetch_user_helper(username: str) -> Dict[str, Union[Optional[str], int]]:
    """Returns the user's entire info

    Args:
        username (str): Email of the user

    Returns:
        http response
    """
    user = User.query.get(username)

    # Return user
    if user:
        access_token, refresh_token = get_access_tokens(username)
        output = user_schema.dump(user)
        output["access_token"] = access_token
        output["refresh_token"] = refresh_token
        return {"user": output, "status": SUCCESS}
    else:
        return {"user": None, "status": BAD_REQUEST}
