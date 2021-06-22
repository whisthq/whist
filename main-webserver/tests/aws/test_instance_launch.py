import random

from app.models import RegionToAmi, db

from app.helpers.command_helpers.ami_upgrade import upgrade_region
from app.helpers.blueprint_helpers.aws.aws_instance_post import do_scale_up_if_necessary

def test_fail_disabled_instance_launch(hijack_ec2_calls, disable_ami):
    call_list = hijack_ec2_calls
    all_amis = RegionToAmi.query.all()
    if len(all_amis) > 0:
        randomized_ami = random.choice(all_amis)
        region_name = randomized_ami.region_name
        disable_ami(region_name=region_name, client_commit_hash=randomized_ami.client_commit_hash)
        do_scale_up_if_necessary(region_name, randomized_ami.ami_id)
        assert len(call_list) == 0

def test_success_enabled_instance_launch(hijack_ec2_calls, enable_ami):
    call_list = hijack_ec2_calls
    all_amis = RegionToAmi.query.all()
    if len(all_amis) > 0:
        randomized_ami = random.choice(all_amis)
        region_name = randomized_ami.region_name
        enable_ami(region_name=region_name, client_commit_hash=randomized_ami.client_commit_hash)
        do_scale_up_if_necessary(region_name, randomized_ami.ami_id, 1)
        assert len(call_list) == 1
        assert call_list[0]["kwargs"]["image_id"] == randomized_ami.ami_id