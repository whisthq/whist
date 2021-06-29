import random, requests

from app.models import RegionToAmi, db
from tests.patches import function

from app.helpers.command_helpers import ami_upgrade
from app.helpers.command_helpers.ami_upgrade import (
    launch_new_ami_buffer,
    perform_upgrade,
    fetch_current_running_instances,
)
from app.helpers.blueprint_helpers.aws.aws_instance_post import do_scale_up_if_necessary

from app.helpers.utils.general.name_generation import generate_name

from app.constants.instance_state_values import (
    DRAINING,
    ACTIVE,
    PRE_CONNECTION,
)


def test_fail_disabled_instance_launch(hijack_ec2_calls, hijack_db, set_amis_state):
    """
    Tests that we won't be able to launch an AMI that is marked as inactive.
    """
    call_list = hijack_ec2_calls
    all_amis = RegionToAmi.query.all()
    if len(all_amis) > 0:
        randomized_ami = random.choice(all_amis)
        region_name = randomized_ami.region_name
        set_amis_state([randomized_ami], False)
        do_scale_up_if_necessary(region_name, randomized_ami.ami_id)
        assert len(call_list) == 0


def test_success_enabled_instance_launch(hijack_ec2_calls, hijack_db, set_amis_state):
    """
    Tests that we should be able to launch an AMI that is marked as active.
    """
    call_list = hijack_ec2_calls
    all_amis = RegionToAmi.query.all()
    if len(all_amis) > 0:
        randomized_ami = random.choice(all_amis)
        region_name = randomized_ami.region_name
        set_amis_state([randomized_ami], True)
        do_scale_up_if_necessary(region_name, randomized_ami.ami_id, 1)
        assert len(call_list) == 1
        assert call_list[0]["kwargs"]["image_id"] == randomized_ami.ami_id


def test_launch_buffer_in_a_region(app, monkeypatch, hijack_ec2_calls, hijack_db):
    """
    `launch_new_ami_buffer` function takes a region name, AMI to ensure that we
    have the preconfigured buffer mandelbox capacity (currently 10) for the given AMI.

    When this function gets invoked during AMI upgrade, there won't be any instances
    running with the newer AMIs.

    This test cases checks that functionality, we trigger the `launch_new_ami_buffer`
    with a new AMI, mock the call to EC2 for spinning up the instances and check that
    we are launching as many instances as preconfigured and with the correct AMI.
    """
    call_list = hijack_ec2_calls
    monkeypatch.setattr(ami_upgrade, "_poll", function(returns=True))
    all_amis = RegionToAmi.query.all()
    if len(all_amis) > 0:
        randomized_ami = random.choice(all_amis)
        randomly_picked_ami_id = randomized_ami.ami_id
        launch_new_ami_buffer(randomized_ami.region_name, randomly_picked_ami_id, app)
        assert len(call_list) == app.config["DEFAULT_INSTANCE_BUFFER"]
        assert call_list[0]["kwargs"]["image_id"] == randomly_picked_ami_id


def test_perform_ami_upgrade(monkeypatch, region_to_ami_map, hijack_db, bulk_instance):
    """
    In this test case, we are testing the whole AMI upgrade flow. This involves the
    following checks
    - Check if we are calling `launch_new_ami_buffer` to ensure that we have enough
    buffer capacity with the new AMIs
        - We have test case to check if the `launch_new_ami_buffer` does launch instances
        so, in this test case we will mock the function and check for only function invocation.
    - Check if we are marking the AMIs that are active before the AMI upgrade are
    marked as inactive.
    - Check if we are marking the newer AMIs are marked as inactive.
    - Mark the instances that are running(in ACTIVE or PRE_CONNECTION) state for draining
    by calling the `drain_and_shutdown` endpoint on the host_service.
    """
    db_call_list = hijack_db

    launch_new_ami_buffer_calls = []

    def _mock_launch_new_ami_buffer(*args, **kwargs):
        launch_new_ami_buffer_calls.append({"args": args, "kwargs": kwargs})

    # We are mocking the `launch_new_ami_buffer` to capture the function calls
    # and check args to ensure that we are upgrading the appropriate region
    # with the correct AMI.
    monkeypatch.setattr(ami_upgrade, "launch_new_ami_buffer", _mock_launch_new_ami_buffer)

    num_running_instances = 10

    def _mock_instance_info_query(*args, **kwargs):
        return [
            bulk_instance(
                instance_name=generate_name("current_running_instance", True),
                status=random.choice([ACTIVE, PRE_CONNECTION]),
            )
            for _ in range(num_running_instances)
        ]

    # We mock `fetch_current_running_instances` to return a list of mock
    # instances that are running with older AMIs. We will be checking that the
    # instances returned are being passed to drain_and_shutdown.
    monkeypatch.setattr(ami_upgrade, "fetch_current_running_instances", _mock_instance_info_query)

    drain_and_shutdown_call_list = []

    def _helper(*args, **kwargs):
        drain_and_shutdown_call_list.append({"args": args, "kwargs": kwargs})
        raise requests.exceptions.RequestException()

    monkeypatch.setattr(requests, "post", _helper)

    region_current_active_ami_map = {}
    current_active_amis = RegionToAmi.query.filter_by(ami_active=True).all()
    for current_active_ami in current_active_amis:
        region_current_active_ami_map[current_active_ami.region_name] = current_active_ami

    regions_to_upgrade = random.sample(region_to_ami_map.keys(), 2)
    new_ami_list = []
    region_to_new_ami_map = {}
    for region in regions_to_upgrade:
        newer_ami = generate_name("new-ami", True)
        new_ami_list.append(newer_ami)
        region_to_new_ami_map[region] = newer_ami

    perform_upgrade(generate_name("new-client-hash", True), region_to_new_ami_map)

    region_wise_new_amis_added_to_db_session = {
        item["args"].region_name: item["args"]
        for item in db_call_list
        if isinstance(item["args"], RegionToAmi)
    }

    for upgraded_region in regions_to_upgrade:
        # Checking that older AMI is marked as inactive.
        assert region_current_active_ami_map[upgraded_region].ami_active is False
        # Checking that newer AMI is being marked as active.
        assert region_wise_new_amis_added_to_db_session[upgraded_region].ami_active is True

    # Checking if all the running instances with older AMIs are being marked for draining.
    assert len(drain_and_shutdown_call_list) == num_running_instances

    for region, new_ami, launch_new_ami_buffer_call in zip(
        regions_to_upgrade, new_ami_list, launch_new_ami_buffer_calls
    ):
        # Iterating through each region that is being upgraded and confirming that
        # we are invoking the `launch_new_ami_buffer` with appropriate args.
        launch_new_ami_buffer_args = launch_new_ami_buffer_call["args"]
        assert launch_new_ami_buffer_args[0] == region
        assert launch_new_ami_buffer_args[1] == new_ami
