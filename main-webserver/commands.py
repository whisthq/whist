from threading import Thread
from json import loads


import requests
from flask import Blueprint, current_app
from sqlalchemy import or_
import click

from app.models import db, RegionToAmi, InstanceInfo
from app.helpers.utils.general.logs import fractal_logger
from app.helpers.blueprint_helpers.aws.aws_instance_post import do_scale_up_if_necessary
from app.helpers.blueprint_helpers.aws.aws_instance_state import _poll
from app.constants.instance_state_values import (
    ACTIVE,
    PRE_CONNECTION,
    DRAINING,
    HOST_SERVICE_UNRESPONSIVE,
)

command_bp = Blueprint("command", __name__)


def _insert_disabled_amis(client_commit_hash, region_to_ami_id_mapping):
    new_disabled_amis = []
    for region_name, ami_id in region_to_ami_id_mapping.items():
        new_ami = RegionToAmi(
            region_name=region_name,
            ami_id=ami_id,
            client_commit_hash=client_commit_hash,
            enabled=False,
            allowed=True,
        )
        new_disabled_amis.append(new_ami)
    db.session.add_all(new_disabled_amis)
    db.session.commit()
    return new_disabled_amis


def upgrade_region(region_name, ami_id, flask_app):
    fractal_logger.debug(f"launching_instances in {region_name} with ami: {ami_id}")
    with flask_app.app_context():
        # TODO: Right now buffer seems to be 1 instance if it is the first of its kind(AMI),
        #       Probably move this to a config.
        force_buffer = 1
        new_instances = do_scale_up_if_necessary(region_name, ami_id, force_buffer)
        for new_instance in new_instances:
            fractal_logger.debug(
                f"Waiting for instance with name: {new_instance.instance_name} to be marked online"
            )
            _poll(new_instance.instance_name)


@command_bp.cli.command("ami_upgrade")
@click.argument("client_commit_hash")
@click.argument("region_to_ami_id_mapping_str")
def ami_upgrade(
    client_commit_hash: str,
    region_to_ami_id_mapping_str: str,
):
    region_to_ami_id_mapping = loads(region_to_ami_id_mapping_str)

    new_disabled_amis = _insert_disabled_amis(client_commit_hash, region_to_ami_id_mapping)

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
        try:
            base_url = f"http://{active_instance.ip}/{current_app.config['HOST_SERVICE_PORT']}"
            requests.post(f"{base_url}/drain_and_shutdown")
            active_instance.status = DRAINING
        except requests.exceptions.RequestException:
            active_instance.status = HOST_SERVICE_UNRESPONSIVE

    for new_disabled_ami in new_disabled_amis:
        new_disabled_ami.enabled = True

    db.session.commit()
