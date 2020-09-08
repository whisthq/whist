from app.constants.http_codes import NOT_FOUND, SUCCESS
from app.models.hardware import UserContainer


def protocol_info(address):
    """Returns information, which is consumed by the protocol, to the client.

    Arguments:
        address: The IP address of the container whose information should be
            returned.
    """

    response = None, NOT_FOUND
    container = UserContainer.query.filter_by(ip=address).first()

    if container:
        response = (
            {
                "allow_autoupdate": container.allow_autoupdate,
                "branch": container.branch,
                "secret_key": "TODO",
                "using_stun": container.using_stun,
            },
            SUCCESS,
        )

    return response
