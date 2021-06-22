import requests
from flask import current_app

from app.models import db, RegionToAmi
from app.helpers.utils.general.logs import fractal_logger
from app.helpers.blueprint_helpers.aws.aws_instance_post import do_scale_up_if_necessary
from app.helpers.blueprint_helpers.aws.aws_instance_state import _poll
from app.constants.instance_state_values import (
    DRAINING,
    HOST_SERVICE_UNRESPONSIVE,
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
