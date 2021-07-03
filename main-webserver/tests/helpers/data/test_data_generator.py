from sqlalchemy.sql.expression import false, true
from app.models import db, RegionToAmi
from app.helpers.utils.general.name_generation import generate_name

from tests.constants import REGION_NAMES, CLIENT_COMMIT_HASH_FOR_TESTING, ACTIVE_AMI, INACTIVE_AMIS


def _generate_region_to_ami_objs(ami_name, client_commit_hash, is_ami_active):
    region_ami_objs = []
    for region_name in REGION_NAMES:
        region_ami_objs.append(
            RegionToAmi(
                region_name=region_name,
                ami_id=ami_name,
                client_commit_hash=client_commit_hash,
                ami_active=is_ami_active,
                region_enabled=True,
            )
        )
    return region_ami_objs


def populate_test_data():
    for inactive_ami in INACTIVE_AMIS:
        db.session.add_all(
            _generate_region_to_ami_objs(
                inactive_ami, generate_name("inactive-client-hash", true), False
            )
        )
    db.session.add_all(
        _generate_region_to_ami_objs(ACTIVE_AMI, CLIENT_COMMIT_HASH_FOR_TESTING, True)
    )
    db.session.commit()
