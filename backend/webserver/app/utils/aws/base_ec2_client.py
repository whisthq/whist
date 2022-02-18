# -*- coding: utf-8 -*-
# This file holds the utility library for EC2 (AWS cloud compute) instance orchestration
# Specifically, it governs instance starting, stopping, and getting
# the IPs of running instances, as well as some useful state checkers.
# Tests live in helpers_tests/aws_tests/test_ec2_client.py, and those tests also demonstrate
# common usage.


import itertools
import time

from typing import Any, Dict, List, Optional
import boto3

from flask import current_app
from app.constants.ec2_instance_states import EC2InstanceState
from app.constants.env_names import PRODUCTION, STAGING, DEVELOPMENT
from app.utils.aws.ec2_userdata import userdata_template
from app.utils.cloud_interface.base_cloud_interface import CloudClient
from app.utils.general.logs import whist_logger


class InstancesNotRunningException(Exception):
    """
    This exception is thrown when instances that are expected to be up
    are not
    """


class EC2Client(CloudClient):
    """
    This class governs everything you need to provision instances on EC2
    Args:
        region_name (str):  which AWS region you're running on
        key_id (Optional[str]): the AWS access key ID to use
        access_key (Optional[str]): the AWS access key going with that key_id
    """

    def __init__(
        self,
        region_name: str,
        key_id: Optional[str] = None,
        access_key: Optional[str] = None,
    ):
        #  We create a new session here to preserve BOTO3 thread safety
        #  See https://github.com/boto/boto3/issues/1592
        boto_session = boto3.session.Session()
        self.ec2_client = boto_session.client(
            "ec2",
            aws_access_key_id=key_id,
            aws_secret_access_key=access_key,
            region_name=region_name,
        )

    def start_instances(
        self,
        image_id: str,
        instance_name: str,
        num_instances: int = 1,
        instance_type: str = "g4dn.12xlarge",
    ) -> List[str]:
        """
        Starts AWS instances with the given properties, returning once started
        Args:
            image_id: which AMI to use
            instance_name: what name the instance should have
            num_instances: how many instances to start
            instance_type: which type of instance (hardwarewise) to start

        Returns: the IDs of the started instances

        """
        # Rather than having some complex launch config, we just set the AWS
        # instance parameters here, at call time.
        # Note that the IamInstanceProfile is set to one created in the
        # Console to allows read-only EC2 access and full S3 access.

        # Get instance profile to launch instances.
        profile = self.get_instance_profile()

        kwargs = {
            "ImageId": image_id,
            "InstanceType": instance_type,
            "MaxCount": num_instances,
            "MinCount": num_instances,
            "TagSpecifications": [
                {
                    "ResourceType": "instance",
                    "Tags": [
                        {"Key": "Name", "Value": instance_name},
                    ],
                },
            ],
            "UserData": userdata_template,
            "IamInstanceProfile": {
                "Arn": profile,
            },
            "InstanceInitiatedShutdownBehavior": "terminate",
        }
        resp = self.ec2_client.run_instances(**kwargs)
        instance_ids = [instance["InstanceId"] for instance in resp["Instances"]]

        return instance_ids

    def stop_instances(self, instance_ids: List[str]) -> Any:
        """
        Turns off all instances passed in
        Args:
            instance_ids: which instances to disable
        """
        resp = self.ec2_client.terminate_instances(InstanceIds=instance_ids)
        whist_logger.info(f"terminating instances {instance_ids} | response {resp}")
        return resp

    def get_instance_states(self, instance_ids: List[str]) -> List[EC2InstanceState]:
        """
        Checks the state of each of the provided instances.

        Args:
            instance_ids: the instances to check

        Returns: a list of EC2InstanceState values, corresponding to the states
            of the provided instances.
        """
        if len(instance_ids) == 0:
            return []

        try:
            resp = self.ec2_client.describe_instances(InstanceIds=instance_ids)
            instance_info = resp["Reservations"][0]["Instances"]
            states = [instance["State"]["Name"] for instance in instance_info]
            return [EC2InstanceState(s) for s in states]
        except Exception:
            # This means that one of the instances provided does not exist. If
            # only one argument was passed in, we know which one's the culprit.
            # If not, we need to iterate through all of them individually and
            # lose the efficiency of making a single call to the AWS API. The
            # alternative would be letting the exception propagate up the call
            # stack, crashing our application. We don't like that.
            if len(instance_ids) == 1:
                return [EC2InstanceState.DOES_NOT_EXIST]
            else:
                individual_results = [self.get_instance_states([i]) for i in instance_ids]
                # Flatten `individual_results` (see https://stackoverflow.com/a/953097/2378475)
                return list(itertools.chain.from_iterable(individual_results))

    def all_running(self, instance_ids: List[str]) -> bool:
        """
        A helper function that returns true if all instances provided are in
        the "running" EC2 state.

        Args:
            instance_ids: which instances to check

        Returns: `true` if all instances are running, `false` otherwise.
        """
        if len(instance_ids) == 0:
            return True

        states = self.get_instance_states(instance_ids)
        return states[0] == EC2InstanceState.RUNNING and len(set(states)) == 1

    def all_not_running(self, instance_ids: List[str]) -> bool:
        """
        A helper function that returns true if all instances provided are in
        the "not running" EC2 state.

        Args:
            instance_ids: which instances to check

        Returns: `true` if all instances are not running, `false` otherwise.
        """
        if len(instance_ids) == 0:
            return True

        states = self.get_instance_states(instance_ids)
        return EC2InstanceState.RUNNING not in set(states)

    def spin_til_instances_running(self, instance_ids: List[str], time_wait: int = 10) -> None:
        """
        Polls AWS every time_wait seconds until all specified instances are
        marked as running by AWS (NOT necessarily fully initialized and ready
        to accept connections yet).
        Args:
            instance_ids: which instances to check
            time_wait: how long to wait between polls

        Returns: None

        """
        while not self.all_running(instance_ids):
            time.sleep(time_wait)

    def spin_til_instances_not_running(self, instance_ids: List[str], time_wait: int = 10) -> None:
        """
        Polls AWS every time_wait seconds until all specified instances are
        marked as not running by AWS.
        Args:
            instance_ids: which instances to check
            time_wait: how long to wait between polls

        Returns: None

        """
        while not self.all_not_running(instance_ids):
            time.sleep(time_wait)

    def get_ip_of_instances(self, instance_ids: List[str]) -> Dict[str, str]:
        """
        Gets the IP addresses of the passed in instances, by ID
        Args:
            instance_ids: what IDs to check

        Returns: a dict mapping instance ID to IP address

        """
        if not self.all_running(instance_ids):
            raise InstancesNotRunningException(str(instance_ids))
        resp = self.ec2_client.describe_instances(InstanceIds=instance_ids)
        instance_info = resp["Reservations"][0]["Instances"]
        resdict = {}
        for instance in instance_info:
            resdict[instance["InstanceId"]] = instance["PublicIpAddress"]
        return resdict

    def get_instance_profile(self) -> str:
        # TODO all all environment instance profiles
        # once we promote Terraform to staging and prod.
        if current_app.config["ENVIRONMENT"] == DEVELOPMENT:
            return "arn:aws:iam::747391415460:instance-profile/EC2DeploymentRoleInstanceProfile"
        elif current_app.config["ENVIRONMENT"] == STAGING:
            return ""
        elif current_app.config["ENVIRONMENT"] == PRODUCTION:
            return ""
        else:
            # Default to dev
            return "arn:aws:iam::747391415460:instance-profile/EC2DeploymentRoleInstanceProfile"
