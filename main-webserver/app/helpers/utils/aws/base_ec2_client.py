from typing import Any, Optional
import boto3



def check_str_param(val: Any, name: str) -> Any:
    """
    Checks the incoming type of a parameter
    Args:
        val: the value to check
        name: the name of the incoming param

    Returns: the value iff it's a string, else raises an exception

    """
    if isinstance(val, str):
        return val
    raise Exception("Value {} was supposed to be a string, but was a {}".format(name, type(val)))


class EC2Client:
    """
    This class governs everything you need to interface with instances on EC2
    Args:
        region_name (Optional[str]):  which AWS region you're running on
        key_id (Optional[str]): the AWS access key ID to use
        access_key (Optional[str]): the AWS access key going with that key_id
        starter_ec2_client (boto3.client): starter log client, used for mocking
        starter_iam_client (boto3.client): starter iam client, used for mocking
    """

    def __init__(
        self,
        region_name: str = "us-east-1",
        key_id: Optional[str] = "",
        access_key: Optional[str] = "",
        starter_ec2_client: Optional[boto3.client] = None,
        starter_iam_client: Optional[boto3.client] = None,
    ):
        self.key_id = check_str_param(key_id, "key_id")
        self.region_name = check_str_param(region_name, "region_name")
        self.access_key = check_str_param(access_key, "access_key")
        self.instances = dict()

        if starter_ec2_client is None:
            self.ec2_client = self.make_client("ec2")
        else:
            self.ec2_client = starter_ec2_client
        if starter_iam_client is None:
            self.iam_client = self.make_client("iam")
        else:
            self.iam_client = starter_iam_client

    def make_client(self, client_type: str) -> Any:
        """
        Constructs an ECS client object with the given params
        Args:
            client_type (str): which ECS client you're trying to produce
        Returns:
            client: the constructed client object
        """
        client = boto3.client(
            client_type,
            aws_access_key_id=(self.key_id if len(self.key_id) > 0 else None),
            aws_secret_access_key=(self.access_key if len(self.access_key) > 0 else None),
            region_name=(self.region_name if len(self.region_name) > 0 else None),
        )
        return client

    def start_instance(self):
        pass

    def stop_instance(self):
        pass

    def spin_til_instance_up(self, instance_id):
        pass

    def spin_til_instance_down(self, instance_id):
        pass
