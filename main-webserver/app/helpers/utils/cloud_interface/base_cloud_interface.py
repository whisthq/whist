import abc
from typing import Dict, List


class CloudClient(metaclass=abc.ABCMeta):
    """
    This class defines the interface all our cloud providers should implement.
    """

    @abc.abstractmethod
    def start_instances(
        self,
        image_id: str,
        instance_name: str,
        num_instances: int = 1,
        instance_type: str = "g3.4xlarge",
    ) -> List[str]:
        """
        Starts AWS instances with the given properties, not returning until they're
        actively running
        Args:
            image_id: which instance image to use
            instance_name: what name the instance should have
            num_instances: how many instances to start
            instance_type: which type of instance (hardwarewise) to start

        Returns: the IDs of the started instances

        """
        raise NotImplementedError

    @abc.abstractmethod
    def stop_instances(self, instance_ids: List[str]) -> None:
        """
        Turns off all instances passed in
        Args:
            instance_ids: which instances to disable
        """
        raise NotImplementedError

    @abc.abstractmethod
    def get_ip_of_instances(self, instance_ids: List[str]) -> Dict[str, str]:
        """
        Gets the IP addresses of the passed in instances, by ID
        Args:
            instance_ids: what IDs to check

        Returns: a dict mapping instance ID to IP address

        """
        raise NotImplementedError
