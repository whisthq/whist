import time

import pytest

from app.helpers.utils.aws.base_ec2_client import EC2Client


def test_single() -> None:
    """
    Tests that starting an instance works as expected
    Returns:
        None
    """
    ec2_client = EC2Client()
    up_start = time.time()
    ids = ec2_client.start_instances("ami-037b96e43364db32c")
    # it takes a moment for AWS to recognize these instances exist
    time.sleep(5)
    assert len(ids) == 1
    assert time.time() - up_start < 20, "start should not be blocking"
    ec2_client.spin_til_instances_up(ids)
    assert ids[0] in ec2_client.get_ip_of_instances(ids).keys()
    down_start = time.time()
    ec2_client.stop_instances(ids)
    assert time.time() - down_start < 20, "stop should not be blocking"
    ec2_client.spin_til_instances_down(ids)
    assert ec2_client.check_if_instances_down(ids)
    with pytest.raises(Exception, match="instances must be up to check IPs"):
        ec2_client.get_ip_of_instances(ids)
