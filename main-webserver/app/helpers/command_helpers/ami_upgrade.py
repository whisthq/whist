from threading import Thread
import requests
from flask import current_app
from sqlalchemy import or_

from app.models import db, RegionToAmi, InstanceInfo
from app.helpers.utils.general.logs import fractal_logger
from app.helpers.blueprint_helpers.aws.aws_instance_post import do_scale_up_if_necessary
from app.helpers.blueprint_helpers.aws.aws_instance_state import _poll
from app.constants.instance_state_values import (
    DRAINING,
    HOST_SERVICE_UNRESPONSIVE,
    ACTIVE,
    PRE_CONNECTION,
)


def insert_new_amis(client_commit_hash, region_to_ami_id_mapping):
    new_amis = []
    for region_name, ami_id in region_to_ami_id_mapping.items():
        new_ami = RegionToAmi(
            region_name=region_name,
            ami_id=ami_id,
            client_commit_hash=client_commit_hash,
            enabled=False,
            allowed=True,
        )
        new_amis.append(new_ami)
    db.session.add_all(new_amis)
    db.session.commit()
    return new_amis


def launch_new_ami_buffer(region_name, ami_id, flask_app):
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


def mark_instance_for_draining(active_instance):
    try:
        base_url = f"http://{active_instance.ip}:{current_app.config['HOST_SERVICE_PORT']}"
        requests.post(f"{base_url}/drain_and_shutdown")
        # Host service would be setting the state in the DB once we call the drain endpoint.
        # However, there is no downside to us setting this as well.
        active_instance.status = DRAINING
    except requests.exceptions.RequestException:
        active_instance.status = HOST_SERVICE_UNRESPONSIVE


def fetch_current_running_instances():
    return (
        db.session.query(InstanceInfo)
        .filter(or_(InstanceInfo.status.like(ACTIVE), InstanceInfo.status.like(PRE_CONNECTION)))
        .all()
    )


def perform_upgrade(client_commit_hash, region_to_ami_id_mapping):
    region_current_active_ami_map = {}
    current_active_amis = RegionToAmi.query.filter_by(enabled=True).all()
    for current_active_ami in current_active_amis:
        region_current_active_ami_map[current_active_ami.region_name] = current_active_ami

    new_amis = insert_new_amis(client_commit_hash, region_to_ami_id_mapping)

    region_wise_upgrade_threads = []
    for region_name, ami_id in region_to_ami_id_mapping.items():
        region_wise_upgrade_thread = Thread(
            target=launch_new_ami_buffer,
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

    for active_instance in fetch_current_running_instances():
        mark_instance_for_draining(active_instance)

    for new_ami in new_amis:
        new_ami.enabled = True
        region_current_active_ami_map[new_ami.region_name].enabled = False

    db.session.commit()
