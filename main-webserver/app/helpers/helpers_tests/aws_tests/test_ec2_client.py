import time

import pytest

from app.helpers.utils.aws.base_ec2_client import EC2Client, InstancesNotRunningException


def test_single() -> None:
    """
    Tests that starting an instance works as expected
    Returns:
        None
    """
    # first, we test that instances start as expected
    ec2_client = EC2Client("us-east-1")
    up_start = time.time()
    ids = ec2_client.start_instances(
        "ami-0dd76f917833aac4b", "test-instance", instance_type="t2.micro"
    )
    # it takes a moment for AWS to recognize these instances exist
    time.sleep(5)

    # check that only one instance started, and that the req is async
    assert len(ids) == 1
    assert time.time() - up_start < 20, "start should not be blocking"

    # Now, poll til it's live
    ec2_client.spin_til_instances_up(ids)

    # And check that the instance is live
    assert ids[0] in ec2_client.get_ip_of_instances(ids).keys()

    # Now, we test stop in the same way we tested start
    down_start = time.time()
    ec2_client.stop_instances(ids)
    assert time.time() - down_start < 20, "stop should not be blocking"
    ec2_client.spin_til_instances_down(ids)
    assert ec2_client.check_if_instances_down(ids)
    with pytest.raises(InstancesNotRunningException):
        ec2_client.get_ip_of_instances(ids)
