import random

from app.models import RegionToAmi, db
from tests.patches import function

from app.helpers.command_helpers import ami_upgrade
from app.helpers.command_helpers.ami_upgrade import launch_new_ami_buffer, perform_upgrade, requests
from app.helpers.blueprint_helpers.aws.aws_instance_post import do_scale_up_if_necessary

from app.helpers.utils.general.name_generation import generate_name


def test_fail_disabled_instance_launch(hijack_ec2_calls, hijack_db, set_amis_state):
    call_list = hijack_ec2_calls
    all_amis = RegionToAmi.query.all()
    if len(all_amis) > 0:
        randomized_ami = random.choice(all_amis)
        region_name = randomized_ami.region_name
        set_amis_state([randomized_ami], False)
        do_scale_up_if_necessary(region_name, randomized_ami.ami_id)
        assert len(call_list) == 0


def test_success_enabled_instance_launch(hijack_ec2_calls, hijack_db, set_amis_state):
    call_list = hijack_ec2_calls
    all_amis = RegionToAmi.query.all()
    if len(all_amis) > 0:
        randomized_ami = random.choice(all_amis)
        region_name = randomized_ami.region_name
        set_amis_state([randomized_ami], True)
        do_scale_up_if_necessary(region_name, randomized_ami.ami_id, 1)
        assert len(call_list) == 1
        assert call_list[0]["kwargs"]["image_id"] == randomized_ami.ami_id


def test_region_upgrade(app, monkeypatch, hijack_ec2_calls, hijack_db):
    call_list = hijack_ec2_calls
    monkeypatch.setattr(ami_upgrade, "_poll", function(returns=True))
    all_amis = RegionToAmi.query.all()
    if len(all_amis) > 0:
        randomized_ami = random.choice(all_amis)
        randomly_picked_ami_id = randomized_ami.ami_id
        launch_new_ami_buffer(randomized_ami.region_name, randomly_picked_ami_id, app)
        assert len(call_list) == 1
        assert call_list[0]["kwargs"]["image_id"] == randomly_picked_ami_id


def test_perform_ami_upgrade(monkeypatch, region_to_ami_map, hijack_db):
    db_call_list = hijack_db

    launch_new_ami_buffer_calls = []

    def _mock_launch_new_ami_buffer(*args, **kwargs):
        launch_new_ami_buffer_calls.append({"args": args, "kwargs": kwargs})

    monkeypatch.setattr(ami_upgrade, "launch_new_ami_buffer", _mock_launch_new_ami_buffer)

    region_current_active_ami_map = {}
    current_active_amis = RegionToAmi.query.filter_by(enabled=True).all()
    for current_active_ami in current_active_amis:
        region_current_active_ami_map[current_active_ami.region_name] = current_active_ami

    regions_to_upgrade = random.sample(region_to_ami_map.keys(), 2)

    region_to_new_ami_map = {}
    for region in regions_to_upgrade:
        region_to_new_ami_map[region] = generate_name("new-ami", True)

    perform_upgrade(generate_name("new-client-hash", True), region_to_new_ami_map)

    region_wise_new_amis_added_to_db_session = {
        item["args"].region_name: item["args"] for item in db_call_list
    }

    for upgraded_region in regions_to_upgrade:
        assert region_current_active_ami_map[upgraded_region].enabled is False
        assert region_wise_new_amis_added_to_db_session[upgraded_region].enabled is True
