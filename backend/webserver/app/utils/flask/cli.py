"""Custom Flask CLI extensions

This module contains blueprints that create Flask CLI (sub)commands when they are registered to a
Flask application. See https://flask.palletsprojects.com/en/2.0.x/cli/#custom-commands.

Example usage::

   # wsgi.py

   from flask import Flask
   from app.utils.flask.cli import command_bp

   app = Flask(__name__)
   app.register_blueprint(command_bp)

Once a blueprint that defines custom CLI commands has been registered to a Flask application, the
custom commands will appear in the ``flask help`` output and can be called as subcommands of
``flask``. For example, the commands defined by the ``command_bp`` blueprint can be called with::

   $ flask ami create_buffers
   ...
   $ flask ami swap_over_buffers
   ...
   $ flask ami generate_test_data
   ...
"""

import json
import os
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timedelta, timezone
from typing import Dict, List

import boto3
import click
from flask import Blueprint, current_app
from flask.cli import AppGroup
from sqlalchemy.sql.expression import exists

from app.database.models.cloud import (
    db,
    InstanceInfo,
    MandelboxHostState,
    MandelboxInfo,
    RegionToAmi,
)
from app.helpers.ami.ami_upgrade import create_ami_buffer, swapover_amis
from app.helpers.aws.aws_instance_post import _get_num_new_instances

# This blueprint creates CLI commands that can be used to manipulate AMIs when it is registered to
# a Flask application.
command_bp = Blueprint("command", __name__, cli_group="ami")
region_cli = AppGroup("region", short_help="Manage compute capacity")


@command_bp.cli.command("create_buffers")  # type: ignore
@click.argument("client_commit_hash")
@click.argument("region_to_ami_id_mapping_str")
def create_buffers(client_commit_hash: str, region_to_ami_id_mapping_str: str) -> None:
    """
    This function creates buffers of instances for a given set of AMIs.
    Args:
        client_commit_hash: The commit hash of the client that will be compatible with the
                            AMIs passed in as the second argument
        region_to_ami_id_mapping_str: Stringified map for Region -> AMI, that are compatible
                            with the commit_hash passed in as the first argument.
    Returns:
        None
    """

    current_app.config["WHIST_ACCESS_TOKEN"] = os.environ["WHIST_ACCESS_TOKEN"]
    region_to_ami_id_mapping: Dict[str, str] = json.loads(region_to_ami_id_mapping_str)
    new_amis, amis_failed = create_ami_buffer(client_commit_hash, region_to_ami_id_mapping)

    print(f"::set-output name=new_amis::{json.dumps(new_amis)}")
    print(f"::set-output name=amis_failed::{json.dumps(amis_failed)}")


@command_bp.cli.command("swap_over_buffers")  # type: ignore
@click.argument("new_amis")
@click.argument("amis_failed")
def swap_over_buffers(new_amis: str, amis_failed: str) -> None:
    """
    This function sets the new AMIs to active, the old AMIs to inactive,
    and drains all previously active instances.
    Args:
        new_amis: Stringified list of new AMIs.
        amis_failed: indicates if any AMI failed to create a buffer.
    Returns:
        None
    """

    current_app.config["WHIST_ACCESS_TOKEN"] = os.environ["WHIST_ACCESS_TOKEN"]
    new_amis_list: List[str] = json.loads(new_amis)
    amis_failed_bool: bool = json.loads(amis_failed)
    swapover_amis(new_amis_list, amis_failed_bool)


@region_cli.command()  # type: ignore[misc]
@click.argument("provider", required=True)
@click.argument("region", required=True)
def cleanup(provider: str, region: str) -> None:  # pylint: disable=unused-argument
    """Terminate empty instances.

    DRAINING instances that have no connected clients should be terminated. The purpose of this
    operation is to save money and minimize clutter in the database.
    """

    ec2 = boto3.resource("ec2")
    instances = InstanceInfo.query.filter(
        InstanceInfo.status == MandelboxHostState.DRAINING,
        ~(exists().where(MandelboxInfo.instance_name == InstanceInfo.instance_name)),
    )

    # Terminate each instance individually rather than batch-terminating them. If a single instance
    # in a batch-termination request cannot be terminated, the entire request will fail.
    with ThreadPoolExecutor() as pool:
        futures = []

        for instance in instances:
            future = pool.submit(ec2.Instance(instance.instance_name).terminate)
            future.add_done_callback(db.session.delete(instance))
            futures.append(future)

    for i in range(len(futures)):
        try:
            futures[i]
        except:
            # The instance could not be terminated
            pass
        else:
            # The instance was terminated successfully
            pass


@region_cli.command("scale-down")  # type: ignore[misc]
@click.argument("provider", required=True)
@click.argument("region", required=True)
def scale_down(provider: str, region: str) -> None:  # pylint: disable=unused-argument
    """Drain extraneous instances.

    Extraneous instances include instances that were launched from old virtual machine images and
    surplus ACTIVE instances.
    """

    recent = (datetime.now(timezone.utc) - timedelta(hours=1)).timestamp()
    active_ami_id = RegionToAmi.query.filter_by(region_name=region, ami_active=True).one().ami_id
    count = _get_num_new_instances(region, active_ami_id)

    InstanceInfo.query.filter(  # Instances launched from old virtual machine images
        InstanceInfo.aws_ami_id != active_ami_id, InstanceInfo.creation_time_utc_unix_ms < recent
    ).union(
        InstanceInfo.query.filter(  # Surplus capacity
            InstanceInfo.status == MandelboxHostState.ACTIVE,
            InstanceInfo.aws_ami_id == active_ami_id,
            ~(exists().where(MandelboxInfo.instance_name == InstanceInfo.instance_name)),
        ).limit(abs(min(count, 0)))
    ).update(
        {"status": MandelboxHostState.DRAINING}
    )
