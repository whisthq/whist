# This file holds the utility library for EC2 (AWS cloud compute) instance orchestration
# Specifically, it governs instance starting, stopping, and getting
# the IPs of running instances, as well as some useful state checkers.
# Tests live in helpers_tests/aws_tests/test_ec2_client.py, and those tests also demonstrate
# common usage.


import time

from typing import Dict, List, Optional
import boto3

from app.helpers.utils.aws.ec2_userdata.no_ecs_userdata import userdata_template
from app.helpers.utils.cloud_interface.base_cloud_interface import CloudClient


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
        # rather than having some complex launch config, just make the AWS instance
        # parameters at call time
        kwargs = {
            "KeyName": "savvy-test-us-east-1-keypair",
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
            "IamInstanceProfile": {"Name": "auto_scaling_instance_profile"},
            "InstanceInitiatedShutdownBehavior": "terminate",
        }
        resp = self.ec2_client.run_instances(**kwargs)
        instance_ids = [instance["InstanceId"] for instance in resp["Instances"]]

        return instance_ids

    def stop_instances(self, instance_ids: List[str]) -> None:
        """
        Turns off all instances passed in
        Args:
            instance_ids: which instances to disable
        """
        self.ec2_client.terminate_instances(InstanceIds=instance_ids)

    def check_if_instances_up(self, instance_ids: List[str]) -> bool:
        """
        Checks whether a given set of instances are running
        Args:
            instance_ids: the instances to check

        Returns: a boolean corresponding to whether every instance is live

        """
        resp = self.ec2_client.describe_instances(InstanceIds=instance_ids)
        instance_info = resp["Reservations"][0]["Instances"]
        states = [instance["State"]["Name"] for instance in instance_info]
        if all(state == "running" for state in states):
            return True
        return False

    def check_if_instances_down(self, instance_ids: List[str]) -> bool:
        """
        Checks whether a given set of instances are stopped
        Args:
                instance_ids: the instances to check

        Returns: a boolean corresponding to whether every instance is dead`

        """
        resp = self.ec2_client.describe_instances(InstanceIds=instance_ids)
        instance_info = resp["Reservations"][0]["Instances"]
        states = [instance["State"]["Name"] for instance in instance_info]
        if any(state == "running" for state in states):
            return False
        return True

    def spin_til_instances_up(self, instance_ids: List[str], time_wait=10) -> None:
        """
        Polls AWS every time_wait seconds until all specified instances are up
        Args:
            instance_ids: which instances to check
            time_wait: how long to wait between polls

        Returns: None

        """
        while not self.check_if_instances_up(instance_ids):
            time.sleep(time_wait)

    def spin_til_instances_down(self, instance_ids: List[str], time_wait=10) -> None:
        """
        Polls AWS every time_wait seconds until all specified instances are up
        Args:
            instance_ids: which instances to check
            time_wait: how long to wait between polls

        Returns: None

        """
        while not self.check_if_instances_down(instance_ids):
            time.sleep(time_wait)

    def get_ip_of_instances(self, instance_ids: List[str]) -> Dict[str, str]:
        """
        Gets the IP addresses of the passed in instances, by ID
        Args:
            instance_ids: what IDs to check

        Returns: a dict mapping instance ID to IP address

        """
        if not self.check_if_instances_up(instance_ids):
            raise InstancesNotRunningException(str(instance_ids))
        resp = self.ec2_client.describe_instances(InstanceIds=instance_ids)
        instance_info = resp["Reservations"][0]["Instances"]
        resdict = dict()
        for instance in instance_info:
            resdict[instance["InstanceId"]] = instance["PublicIpAddress"]
        return resdict
