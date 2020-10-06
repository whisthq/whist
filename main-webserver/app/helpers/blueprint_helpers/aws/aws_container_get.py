from app.constants.http_codes import NOT_FOUND, SUCCESS
from app.models.hardware import UserContainer


def protocol_info(address, port):
    """Returns information, which is consumed by the protocol, to the client.

    Arguments:
        address: The IP address of the container whose information should be
            returned.
    """

    response = None, NOT_FOUND
    container = UserContainer.query.filter_by(ip=address, port_32262=port).first()

    if container:
        response = (
            {
                "allow_autoupdate": container.allow_autoupdate,
                "branch": container.branch,
                "secret_key": container.secret_key.hex(),
                "using_stun": container.using_stun,
            },
            SUCCESS,
        )

    return response
