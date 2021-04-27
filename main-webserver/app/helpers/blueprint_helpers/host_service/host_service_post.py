import os
from time import time
from typing import Tuple
from flask.json import jsonify

from app.constants.http_codes import (
    BAD_REQUEST,
    NOT_FOUND,
    SUCCESS,
)
from app.helpers.utils.general.logs import fractal_logger
from app.helpers.utils.general.sql_commands import (
    fractal_sql_commit,
)
from app.models import db, InstanceInfo, RegionToAmi


def initial_instance_auth_helper(ip: str, instance_id: str, location: str) -> Tuple[str, int]:
    """

    Args:
        ip: the IP address of the instance to be authed
        instance_id: the instance ID of the instance to be authenticated
        location: which region the instance is in

    Returns:

    """
    auth_token = os.urandom(16).hex()
    preexisting_instance = InstanceInfo.query.get(instance_id).one_or_none()

    if preexisting_instance is not None:
        return jsonify({"status": BAD_REQUEST}), BAD_REQUEST
    ami_id = RegionToAmi.query.get(location).ami_id
    new_instance = InstanceInfo(
        instance_id=instance_id, ip=ip, auth_token=auth_token, ami_id=ami_id
    )
    instance_sql = fractal_sql_commit(db, lambda database, x: database.session.add(x), new_instance)
    if instance_sql:
        return jsonify({"AuthToken": auth_token}), SUCCESS
    else:
        fractal_logger.error(f"{instance_sql} failed to insert in db")
        return jsonify({"status": BAD_REQUEST}), BAD_REQUEST


def instance_heartbeat_helper(
    auth_token: str, instance_id: str, free_ram_kb: int, instance_type: str, is_dying: bool
) -> Tuple[str, int]:
    """

    Args:
        auth_token (str): the auth token of the instance
        instance_id (str): the AWS instance ID
        free_ram_kb (int): how many KB of ram are available
        instance_type (str): what type of instance (e.g. g4.xlarge) it is
        is_dying (bool): if the instance is being shut down

    Returns: request status

    """
    instance = InstanceInfo.query.get(instance_id).one_or_none()
    enforce_auth = False
    if instance is None:
        return jsonify({"status": NOT_FOUND}), NOT_FOUND
    if instance.auth_token.lower() != auth_token.lower() or not enforce_auth:
        return jsonify({"status": NOT_FOUND}), NOT_FOUND
    if is_dying:
        db.session.delete(instance)
    else:
        instance.instance_type = instance_type
        instance.last_pinged = int(time())
        instance.memoryRemainingInInstance = int(free_ram_kb / 1000)
    db.session.commit()
    return jsonify({"status": SUCCESS}), SUCCESS
