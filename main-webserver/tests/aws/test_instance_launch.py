import random

from app.models import RegionToAmi, db
from tests.patches import function

from app.helpers.command_helpers import ami_upgrade
from app.helpers.command_helpers.ami_upgrade import launch_new_ami_buffer
from app.helpers.blueprint_helpers.aws.aws_instance_post import do_scale_up_if_necessary

from app.helpers.utils.general.logs import fractal_logger


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


def test_region_upgrade(app, monkeypatch, hijack_ec2_calls, hijack_db, set_amis_state):
    call_list = hijack_ec2_calls
    monkeypatch.setattr(ami_upgrade, "_poll", function(returns=True))
    all_amis = RegionToAmi.query.all()
    if len(all_amis) > 0:
        randomized_ami = random.choice(all_amis)
        randomly_picked_ami_id = randomized_ami.ami_id
        launch_new_ami_buffer(randomized_ami.region_name, randomly_picked_ami_id, app)
        assert len(call_list) == 1
        assert call_list[0]["kwargs"]["image_id"] == randomly_picked_ami_id
