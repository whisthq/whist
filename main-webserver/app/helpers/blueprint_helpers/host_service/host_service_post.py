import os
from time import time
from typing import Tuple
from flask import Response
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
from app.models import db, InstanceInfo


def initial_instance_auth_helper(
    ip: str, instance_id: str, instance_type: str, location: str, ami_id: str
) -> Tuple[Response, int]:
    """

    Args:
        ip: the IP address of the instance to be authenticated
        instance_id: the instance ID of the instance to be authenticated
        instance_type: what type of hardware the instance is on
        location: which region the instance is in
        ami_id: what AMI the instance is running

    Returns: whether the request succeeded or failed, plus the response code

    """
    auth_token = os.urandom(16).hex()
    preexisting_instance = InstanceInfo.query.get(instance_id)

    # Make sure there's no instance with this ID in the DB already
    if preexisting_instance is not None:
        return jsonify({"status": BAD_REQUEST}), BAD_REQUEST

    # Then create a row for the instance in the DB
    new_instance = InstanceInfo(
        instance_id=instance_id,
        ip=ip,
        auth_token=auth_token,
        instance_type=instance_type,
        ami_id=ami_id,
        location=location,
        status="ACTIVE",
    )
    instance_sql = fractal_sql_commit(db, lambda database, x: database.session.add(x), new_instance)
    if instance_sql:
        return jsonify({"AuthToken": auth_token}), SUCCESS
    else:
        fractal_logger.error(f"{instance_sql} failed to insert in db")
        return jsonify({"status": BAD_REQUEST}), BAD_REQUEST


def instance_heartbeat_helper(
    auth_token: str, instance_id: str, free_ram_kb: int, is_dying: bool
) -> Tuple[Response, int]:
    """

    Args:
        auth_token (str): the auth token of the instance
        instance_id (str): the AWS instance ID
        free_ram_kb (int): how many KB of ram are available
        is_dying (bool): if the instance is being shut down

    Returns: whether the request succeeded or failed, plus the response code

    """
    instance = InstanceInfo.query.get(instance_id)
    if instance is None:
        return jsonify({"status": NOT_FOUND}), NOT_FOUND
    if instance.auth_token.lower() != auth_token.lower():
        return jsonify({"status": NOT_FOUND}), NOT_FOUND
    # If the instance says it's dying, get rid of it
    if is_dying:
        db.session.delete(instance)
    else:
        instance.last_pinged = int(time())
        instance.memoryRemainingInInstanceInMb = int(free_ram_kb / 1000)
    db.session.commit()
    return jsonify({"status": SUCCESS}), SUCCESS
