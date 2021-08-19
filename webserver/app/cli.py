"""Custom Flask CLI extensions

This module contains blueprints that create Flask CLI (sub)commands when they are registered to a
Flask application. See https://flask.palletsprojects.com/en/2.0.x/cli/#custom-commands.

Example usage::

   # wsgi.py

   from flask import Flask
   from app.cli import command_bp

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
from typing import Dict, List

import click
from flask import Blueprint, current_app

from app.helpers.blueprint_helpers.aws.aws_instance_post import (
    try_scale_down_if_necessary_all_regions,
    check_and_handle_lingering_instances,
)
from app.helpers.command_helpers.ami_upgrade import create_ami_buffer, swapover_amis
from tests.helpers.data.test_data_generator import populate_test_data

# This blueprint creates CLI commands that can be used to manipulate AMIs when it is registered to
# a Flask application.
command_bp = Blueprint("command", __name__, cli_group="ami")

# This blueprint creates the flask compute subcommand when registered to a Flask application.
# Further subcommands under flask compute allow us to manipulate compute resources.
compute_bp = Blueprint("compute", __name__)


@command_bp.cli.command("create_buffers")
@click.argument("client_commit_hash")
@click.argument("region_to_ami_id_mapping_str")
def create_buffers(
    client_commit_hash: str,
    region_to_ami_id_mapping_str: str,
) -> None:
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
    region_to_ami_id_mapping: Dict[str, str] = json.loads(region_to_ami_id_mapping_str)

    new_amis = create_ami_buffer(client_commit_hash, region_to_ami_id_mapping)
    print(f"::set-output name=new_amis::{json.dumps(new_amis)}")


@command_bp.cli.command("swap_over_buffers")
@click.argument("new_amis")
def swap_over_buffers(
    new_amis: str,
):
    """
    This function sets the new AMIs to active, the old AMIs to inactive,
    and drains all previously active instances.
    Args:
        new_amis: Stringified list of new AMIs
    Returns:
        None
    """
    new_amis_list: List[str] = json.loads(new_amis)
    swapover_amis(new_amis_list)


@command_bp.cli.command("generate_test_data")
def _generate_test_data():
    """Generate data required for running test cases.

    This command also populates the database with this data.
    """

    populate_test_data()


# In @owenniles's opinion, all CLI commands should contain hyphens rather than underscores. The
# pattern of using hyphens over underscores when necessary is prevalent in established CLIs.
@compute_bp.cli.command("scale-down-instances")
def scale_down() -> None:
    """Scale compute resources down to the minimum required levels in all regions.

    Schedule this command to run periodically to ensure that we are not paying for too much more
    compute capacity than we need at any given time.
    """

    current_app.config["FRACTAL_ACCESS_TOKEN"] = os.environ["FRACTAL_ACCESS_TOKEN"]
    try_scale_down_if_necessary_all_regions()


@compute_bp.cli.command("prune-lingering-instances")
def prune() -> None:
    """Identify and terminate compute instances left over from past deployments.

    Schedule this command to run periodically to ensure that we are not paying for too much more
    compute capacity than we need at any given time.
    """

    current_app.config["FRACTAL_ACCESS_TOKEN"] = os.environ["FRACTAL_ACCESS_TOKEN"]
    check_and_handle_lingering_instances()
