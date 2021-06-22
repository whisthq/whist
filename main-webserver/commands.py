from threading import Thread
from json import loads


import click
from flask import Blueprint, current_app
from sqlalchemy import or_

from app.models import db, InstanceInfo
from app.helpers.utils.general.logs import fractal_logger
from app.helpers.command_helpers.ami_upgrade import (
    insert_disabled_amis,
    upgrade_region,
    mark_instance_for_draining,
)
from app.constants.instance_state_values import ACTIVE, PRE_CONNECTION


command_bp = Blueprint("command", __name__)


@command_bp.cli.command("ami_upgrade")
@click.argument("client_commit_hash")
@click.argument("region_to_ami_id_mapping_str")
def ami_upgrade(
    client_commit_hash: str,
    region_to_ami_id_mapping_str: str,
):
    region_to_ami_id_mapping = loads(region_to_ami_id_mapping_str)

    new_disabled_amis = insert_disabled_amis(client_commit_hash, region_to_ami_id_mapping)

    region_wise_upgrade_threads = []
    for region_name, ami_id in region_to_ami_id_mapping.items():
        region_wise_upgrade_thread = Thread(
            target=upgrade_region,
            args=(
                region_name,
                ami_id,
            ),
            # current_app is a proxy for app object, so `_get_current_object` method
            # should be used to fetch the application object to be passed to the thread.
            kwargs={"flask_app": current_app._get_current_object()},
        )
        region_wise_upgrade_threads.append(region_wise_upgrade_thread)
        region_wise_upgrade_thread.start()

    for region_wise_upgrade_thread in region_wise_upgrade_threads:
        region_wise_upgrade_thread.join()

    active_instances = (
        db.session.query(InstanceInfo)
        .filter(or_(InstanceInfo.status.like(ACTIVE), InstanceInfo.status.like(PRE_CONNECTION)))
        .all()
    )

    for active_instance in active_instances:
        mark_instance_for_draining(active_instance)

    for new_disabled_ami in new_disabled_amis:
        new_disabled_ami.enabled = True

    db.session.commit()
