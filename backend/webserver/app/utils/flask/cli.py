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
``flask``. For example, the commands defined by the ``compute_bp`` blueprint can be called with::

   $ flask compute scale-down-instances
   ...
   $ flask compute prune-lingering-instances
   ...
   $ flask compute clean-old-commit-hash-instances
   ...
"""

import json
import os
from typing import Dict, List

import click
from flask import Blueprint, current_app

from app.helpers.aws.aws_instance_post import (
    try_scale_down_if_necessary_all_regions,
    check_and_handle_lingering_instances,
    check_and_handle_instances_with_old_commit_hash,
)

# This blueprint creates the flask compute subcommand when registered to a Flask application.
# Further subcommands under flask compute allow us to manipulate compute resources.
compute_bp = Blueprint("compute", __name__)


# In @owenniles's opinion, all CLI commands should contain hyphens rather than underscores. The
# pattern of using hyphens over underscores when necessary is prevalent in established CLIs.
@compute_bp.cli.command("scale-down-instances")  # type: ignore
def scale_down() -> None:
    """Scale compute resources down to the minimum required levels in all regions.

    Schedule this command to run periodically to ensure that we are not paying for too much more
    compute capacity than we need at any given time.
    """

    current_app.config["WHIST_ACCESS_TOKEN"] = os.environ["WHIST_ACCESS_TOKEN"]
    try_scale_down_if_necessary_all_regions()


@compute_bp.cli.command("prune-lingering-instances")  # type: ignore
def prune() -> None:
    """Identify and terminate compute instances left over from past deployments.

    Schedule this command to run periodically to ensure that we are not paying for too much more
    compute capacity than we need at any given time.
    """

    current_app.config["WHIST_ACCESS_TOKEN"] = os.environ["WHIST_ACCESS_TOKEN"]
    check_and_handle_lingering_instances()


@compute_bp.cli.command("clean-old-commit-hash-instances")  # type: ignore
def clean() -> None:
    """Identify and drain compute instances with old commit hash.

    Schedule this command to run periodically to ensure that we are not paying for too much more
    compute capacity than we need at any given time.
    """

    current_app.config["WHIST_ACCESS_TOKEN"] = os.environ["WHIST_ACCESS_TOKEN"]
    check_and_handle_instances_with_old_commit_hash()
