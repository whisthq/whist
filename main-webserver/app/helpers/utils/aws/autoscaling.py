"""Low-level helper functions that interact directly with AWS AutoScaling."""

import boto3
import botocore


def delete_autoscaling_group(autoscaling_group, region):
    """Delete the specified auto-scaling group.

    Args:
        autoscaling_groups: The short name of the autoscaling group to delete as a string.
        region: The AWS region in which the auto-scaling group is located (e.g. "us-east-1").

    Returns:
        None
    """

    autoscaling_client = boto3.client("autoscaling", region_name=region)

    try:
        autoscaling_client.delete_auto_scaling_group(
            AutoScalingGroupName=autoscaling_group,
            ForceDelete=True,
        )
    except botocore.exceptions.ClientError as error:
        if error.response["Error"]["Code"] == "ValidationError":
            # Don't worry if the auto-scaling group doesn't exist.
            pass


def delete_launch_configuration(launch_configuration, region):
    """Delete the specified launch configuration.

    Args:
        launch_configuration: The short name of the launch configuration as a string.
        region: The region in which the launch configuration is located as a string (e.g.
            "us-east-1")

    Returns:
        None
    """

    autoscaling_client = boto3.client("autoscaling", region_name=region)

    try:
        autoscaling_client.delete_launch_configuration(
            LaunchConfigurationName=launch_configuration,
        )
    except botocore.exceptions.ClientError as error:
        if error.response["Error"]["Code"] == "ValidationError":
            # This probably means that the launch configuration was already deleted and doesn't
            # exist anymore. Don't freak out.
            pass


def get_launch_configuration(autoscaling_group, region):
    """Retrieve the name of an auto-scaling group's underlying launch configuration.

    Args:
        autoscaling_group: The short name of the auto-scaling group as a string.
        region: The region in whicht the auto-scaling group is located as a string (e.g.
            "us-east-1")

    Returns:
        The short name of the auto-scaling group's launch configuration as a string or None if the
        specified auto-scaling group doesn't exist.
    """

    autoscaling_client = boto3.client("autoscaling", region_name=region)
    launch_configuration = None
    autoscaling_groups = autoscaling_client.describe_auto_scaling_groups(
        AutoScalingGroupNames=(autoscaling_group,)
    )["AutoScalingGroups"]

    assert len(autoscaling_groups) < 2  # Sanity check

    # It's fine if the list of auto-scaling groups is empty. That just means that the auto-scaling
    # group in question has already been deleted. If it hasn't already been deleted, grab the name
    # of its launch configuration.
    if len(autoscaling_groups) == 1:
        launch_configuration = autoscaling_groups[0]["LaunchConfigurationName"]

    return launch_configuration
