# -*- coding: utf-8 -*-
from typing import Any, Callable, Dict, List
import random

from flask import Flask
from pytest import MonkeyPatch

from app.database.models.cloud import RegionToAmi, db, InstanceInfo, MandelboxHostState
from app.helpers.ami import ami_upgrade
from app.helpers.ami.ami_upgrade import (
    launch_new_ami_buffer,
    insert_new_amis,
    create_ami_buffer,
    swapover_amis,
)
from app.helpers.aws import aws_instance_post
from app.utils.aws.base_ec2_client import EC2Client
from app.utils.general.name_generation import generate_name
from app.constants.ec2_instance_states import EC2InstanceState
from tests.patches import function


def test_prior_ami(db_session: db.session) -> None:  # pylint: disable=unused-argument
    all_amis = RegionToAmi.query.all()
    if len(all_amis) > 0:
        randomized_ami = random.choice(all_amis)
        region_name = randomized_ami.region_name
    else:
        region_name = "us-east-1"
    client_commit_hash = "test-client-commit-hash"
    prior_ami = RegionToAmi(
        region_name=region_name,
        client_commit_hash=client_commit_hash,
        ami_id="prior-ami-us-east-1",
        ami_active=False,
    )
    second_region = "us-east-2" if region_name != "us-east-2" else "us-east-1"
    db.session.add(prior_ami)
    db.session.commit()
    region_to_ami_map = {region_name: f"new-ami-{region_name}", second_region: "new-ami-us-east-2"}
    mixed_ami_ids = [ami.ami_id for ami in insert_new_amis(client_commit_hash, region_to_ami_map)]
    print(mixed_ami_ids)
    assert "new-ami-us-east-2" in mixed_ami_ids, "failed to insert new AMI"


def test_fail_disabled_instance_launch(
    app: Flask,
    hijack_ec2_calls: List[Dict[str, Any]],
    hijack_db: List[Dict[str, Any]],  # pylint: disable=unused-argument
    set_amis_state: Callable[[List[RegionToAmi], bool], None],
) -> None:
    """
    Tests that we won't be able to launch an AMI that is marked as inactive.
    """
    call_list = hijack_ec2_calls
    all_amis = RegionToAmi.query.all()
    if len(all_amis) > 0:
        randomized_ami = random.choice(all_amis)
        region_name = randomized_ami.region_name
        set_amis_state([randomized_ami], False)
        aws_instance_post.do_scale_up_if_necessary(
            region_name, randomized_ami.ami_id, flask_app=app
        )
        assert len(call_list) == 0


def test_success_enabled_instance_launch(
    app: Flask,
    hijack_ec2_calls: List[Dict[str, Any]],
    hijack_db: List[Dict[str, Any]],  # pylint: disable=unused-argument
    set_amis_state: Callable[[List[RegionToAmi], bool], None],
) -> None:
    """
    Tests that we should be able to launch an AMI that is marked as active.
    """
    call_list = hijack_ec2_calls
    all_amis = RegionToAmi.query.all()
    if len(all_amis) > 0:
        randomized_ami = random.choice(all_amis)
        region_name = randomized_ami.region_name
        set_amis_state([randomized_ami], True)
        aws_instance_post.do_scale_up_if_necessary(
            region_name, randomized_ami.ami_id, 1, flask_app=app
        )
        assert len(call_list) == 1
        assert call_list[0]["kwargs"]["image_id"] == randomized_ami.ami_id


def test_launch_buffer_in_a_region(
    app: Flask,
    monkeypatch: MonkeyPatch,
    hijack_ec2_calls: List[Dict[str, Any]],
    hijack_db: List[Dict[str, Any]],  # pylint: disable=unused-argument
) -> None:
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
    monkeypatch.setattr(
        ami_upgrade, "region_wise_upgrade_threads", [["thread-id-0", True, (), None]]
    )
    all_amis = RegionToAmi.query.all()
    if len(all_amis) > 0:
        randomized_ami = random.choice(all_amis)
        randomly_picked_ami_id = randomized_ami.ami_id
        launch_new_ami_buffer(randomized_ami.region_name, randomly_picked_ami_id, 0, app)
        assert len(call_list) == app.config["DEFAULT_INSTANCE_BUFFER"]
        assert call_list[0]["kwargs"]["image_id"] == randomly_picked_ami_id


def test_perform_ami_upgrade(
    app: Flask,
    monkeypatch: MonkeyPatch,
    region_to_ami_map: Dict[str, str],
    bulk_instance: Callable[..., InstanceInfo],
    db_session: db.session,  # pylint: disable=unused-argument
) -> None:
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
    - Mark the instances that are running (in ACTIVE or PRE_CONNECTION state) for draining.
    """

    monkeypatch.setitem(app.config, "WHIST_ACCESS_TOKEN", "dummy-access-token")

    # We mock the `launch_new_ami_buffer` to capture the function calls
    # and check args to ensure that we are upgrading the appropriate region
    # with the correct AMI.
    launch_new_ami_buffer_calls: List[Dict[str, Any]] = []

    def _mock_launch_new_ami_buffer(*args: Any, **kwargs: Any) -> None:
        launch_new_ami_buffer_calls.append({"args": args, "kwargs": kwargs})
        # region_wise_upgrade_threads is a global variable that stores the
        # status of each threads's success state, we need to mark this as true
        # to indicate that the calls to launch_new_ami_buffer have succeded
        thread_index = args[2]
        thread_status_index = 1
        ami_upgrade.region_wise_upgrade_threads[thread_index][thread_status_index] = True

    monkeypatch.setattr(ami_upgrade, "launch_new_ami_buffer", _mock_launch_new_ami_buffer)

    # We mock `fetch_current_running_instances` to return a list of mock
    # instances that are running with older AMIs. We will be checking that the
    # instances returned are being drained.
    num_running_instances = 10
    random_running_instances = [
        bulk_instance(
            instance_name=generate_name("current_running_instance", True),
            status=random.choice([MandelboxHostState.ACTIVE, MandelboxHostState.PRE_CONNECTION]),
        )
        for _ in range(num_running_instances)
    ]

    def _mock_instance_info_query(
        *args: Any, **kwargs: Any  # pylint: disable=unused-argument
    ) -> List[InstanceInfo]:
        return random_running_instances

    monkeypatch.setattr(ami_upgrade, "fetch_current_running_instances", _mock_instance_info_query)

    # We mock _active_instances to make instances appear active so that we can
    # attempt to drain them.
    def _active_instances(*args: Any, **kwargs: Any) -> str:  # pylint: disable=unused-argument
        return EC2InstanceState.RUNNING

    monkeypatch.setattr(EC2Client, "get_instance_states", _active_instances)

    # We also mock the calls to drain_instance to avoid errors from trying to
    # terminate bogus instances with boto3. The functionality of drain_instance
    # is also tested separately so we can do this with confidence.
    drain_call_set = set()

    def _mock_drain_instance(instance: InstanceInfo) -> None:
        drain_call_set.add(instance.instance_name)

    monkeypatch.setattr(ami_upgrade, "drain_instance", _mock_drain_instance)

    region_current_active_ami_map = {}
    current_active_amis = RegionToAmi.query.filter_by(ami_active=True).all()
    for current_active_ami in current_active_amis:
        region_current_active_ami_map[current_active_ami.region_name] = current_active_ami

    regions_to_upgrade = random.sample(list(region_to_ami_map.keys()), 2)
    new_ami_list = []
    region_to_new_ami_map = {}
    for region in regions_to_upgrade:
        newer_ami = generate_name("new-ami", True)
        new_ami_list.append(newer_ami)
        region_to_new_ami_map[region] = newer_ami

    new_amis, amis_failed = create_ami_buffer(
        generate_name("new-client-hash", True), region_to_new_ami_map
    )
    swapover_amis(new_amis, amis_failed)

    region_wise_new_amis_added_to_db_session = {
        item.region_name: item
        for item in (RegionToAmi.query.filter_by(ami_id=ami_id).first() for ami_id in new_amis)
        if isinstance(item, RegionToAmi)
    }

    for upgraded_region in regions_to_upgrade:
        # Checking that older AMI is marked as inactive.
        assert region_current_active_ami_map[upgraded_region].ami_active is False
        # Checking that newer AMI is being marked as active.
        assert region_wise_new_amis_added_to_db_session[upgraded_region].ami_active is True

    for region, new_ami, launch_new_ami_buffer_call in zip(
        regions_to_upgrade, new_ami_list, launch_new_ami_buffer_calls
    ):
        # Iterating through each region that is being upgraded and confirming that
        # we are invoking the `launch_new_ami_buffer` with appropriate args.
        launch_new_ami_buffer_args = launch_new_ami_buffer_call["args"]
        assert launch_new_ami_buffer_args[0] == region
        assert launch_new_ami_buffer_args[1] == new_ami

    # Check that we did indeed drain all the prior-running instances.
    assert drain_call_set == {x.instance_name for x in random_running_instances}
