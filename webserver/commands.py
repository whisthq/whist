"""commands.py

This file contains the functions that can be invoked through
flask CLI - https://flask.palletsprojects.com/en/2.0.x/cli/

"""
from json import loads, dumps
from typing import Dict, List

import click
from flask import Blueprint


from app.helpers.command_helpers.ami_upgrade import perform_upgrade, swapover_amis
from tests.helpers.data.test_data_generator import populate_test_data

# Blueprint for registering the functions under 'command'
# Reference: https://flask.palletsprojects.com/en/2.0.x/cli/#registering-commands-with-blueprints
# All the functions in this file will be nested under `flask command <sub_command>`
# The <sub_command> can be registered by using the `@command_bp.cli.command(<sub_command>)` decorator
command_bp = Blueprint("command", __name__)


# Registering custom command https://flask.palletsprojects.com/en/2.0.x/cli/#custom-commands


# This function upgrades the AMIs for given client hash and the region.
# Can be invoked through `flask command ami_upgrade <client_commit_hash> <region_to_ami_id_mapping_str>`
@command_bp.cli.command("ami_create_buffers")
@click.argument("client_commit_hash")
@click.argument("region_to_ami_id_mapping_str")
def ami_create_buffers(
    client_commit_hash: str,
    region_to_ami_id_mapping_str: str,
) -> None:
    """
    Args:
        client_commit_hash: The commit hash of the client that will be compatible with the
                            AMIs passed in as the second argument
        region_to_ami_id_mapping_str: Stringified map for Region -> AMI, that are compatible
                            with the commit_hash passed in as the first argument.
    Returns:
        None
    """
    region_to_ami_id_mapping: Dict[str, str] = loads(region_to_ami_id_mapping_str)

    new_amis = perform_upgrade(client_commit_hash, region_to_ami_id_mapping)
    print(f"::set-output name=new_amis::{dumps(new_amis)}")


@command_bp.cli.command("ami_swap_over_buffers")
@click.argument("new_amis")
def ami_swap_over_buffers(
    new_amis: str,
):
    """
    Args:
        new_amis: Stringified list of new AMIs
    Returns:
        None
    """
    new_amis_list: List[str] = loads(new_amis)
    swapover_amis(new_amis_list)


# This function generates data required for running the test cases and populates it in the database.
# Can be invoked through `flask command generate_test_data`
@command_bp.cli.command("generate_test_data")
def _generate_test_data():
    """
    Args:
        None
    Returns:
        None
    """
    populate_test_data()
