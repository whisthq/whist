import os
from typing import Tuple
from flask.json import jsonify

from app.constants.http_codes import (
    BAD_REQUEST,
    SUCCESS,
)
from app.helpers.utils.general.logs import fractal_logger
from app.helpers.utils.general.sql_commands import (
    fractal_sql_commit,
)
from app.models import db, InstanceInfo


def initial_instance_auth_helper(ip: str, instance_id: str) -> Tuple[str, int]:
    """

    Args:
        ip: the IP address of the instance to be authed
        instance_id: the instance ID of the instance to be authenticated

    Returns:

    """
    auth_token = os.urandom(16).hex()
    preexisting_instance = InstanceInfo.query.get(instance_id).one_or_none()

    if preexisting_instance is not None:
        return jsonify({"status": BAD_REQUEST}), BAD_REQUEST
    new_instance = InstanceInfo(instance_id=instance_id, ip=ip, auth_token=auth_token)
    instance_sql = fractal_sql_commit(db, lambda database, x: database.session.add(x), new_instance)
    if instance_sql:
        return jsonify({"AuthToken": auth_token}), SUCCESS
    else:
        fractal_logger.error(f"{instance_sql} failed to insert in db")
        return jsonify({"status": BAD_REQUEST}), BAD_REQUEST


def instance_heartbeat_helper() -> Tuple[str, int]:
    """
    Scaffolding for the function that handles instance heartbeats
    """
