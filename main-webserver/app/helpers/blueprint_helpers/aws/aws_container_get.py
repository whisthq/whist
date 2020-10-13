from app.constants.http_codes import NOT_FOUND, SUCCESS
from app.constants.config import DASHBOARD_USERNAME
from app.models.hardware import UserContainer
from app.helpers.utils.general.tokens import getAccessTokens


def protocol_info(address, port):
    """Returns information, which is consumed by the protocol, to the client.

    Arguments:
        address: The IP address of the container whose information should be
            returned.
    """

    response = None, NOT_FOUND
    container = UserContainer.query.filter_by(ip=address, port_32262=port).first()

    if container:
        access_token, refresh_token = getAccessTokens(DASHBOARD_USERNAME + "@gmail.com")
        response = (
            {
                "allow_autoupdate": container.allow_autoupdate,
                "branch": container.branch,
                "private_key": container.secret_key.hex(),
                "using_stun": container.using_stun,
                "access_token": access_token,
                "refresh_token": refresh_token,
            },
            SUCCESS,
        )

    return response
