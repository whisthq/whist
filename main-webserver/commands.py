from json import loads


import click
from flask import Blueprint


from app.helpers.command_helpers.ami_upgrade import perform_upgrade


command_bp = Blueprint("command", __name__)


@command_bp.cli.command("ami_upgrade")
@click.argument("client_commit_hash")
@click.argument("region_to_ami_id_mapping_str")
def ami_upgrade(
    client_commit_hash: str,
    region_to_ami_id_mapping_str: str,
):
    region_to_ami_id_mapping = loads(region_to_ami_id_mapping_str)

    perform_upgrade(client_commit_hash, region_to_ami_id_mapping)
