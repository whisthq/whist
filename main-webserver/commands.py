"""commands.py

This file contains the functions that can be invoked through 
flask CLI - https://flask.palletsprojects.com/en/2.0.x/cli/

"""
from json import loads


import click
from flask import Blueprint


from app.helpers.command_helpers.ami_upgrade import perform_upgrade

# Blueprint for registering the functions under 'command'
# Reference: https://flask.palletsprojects.com/en/2.0.x/cli/#registering-commands-with-blueprints
# All the functions in this file will be nested under `flask command <sub_command>`
# The <sub_command> can be registered by using the `@command_bp.cli.command(<sub_command>)` decorator
command_bp = Blueprint("command", __name__)


# Registering custom command https://flask.palletsprojects.com/en/2.0.x/cli/#custom-commands
# This function upgrades the AMIs for given client hash and the region.
# Can be invoked through `flask command ami_upgrade <client_commit_hash> <region_to_ami_id_mapping_str>`
@command_bp.cli.command("ami_upgrade")
@click.argument("client_commit_hash")
@click.argument("region_to_ami_id_mapping_str")
def ami_upgrade(
    client_commit_hash: str,
    region_to_ami_id_mapping_str: str,
):
    """
    Args:
        client_commit_hash: The commit hash of the client that will be compatible with the
                            AMIs passed in as the second argument
        region_to_ami_id_mapping_str: Stringified map for Region -> AMI, that are compatible
                            with the commit_hash passed in as the first argument.
    Returns:
        None
    """
    region_to_ami_id_mapping = loads(region_to_ami_id_mapping_str)

    perform_upgrade(client_commit_hash, region_to_ami_id_mapping)
