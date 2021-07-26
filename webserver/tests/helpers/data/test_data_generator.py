from sqlalchemy.sql.expression import true
from app.models import db, RegionToAmi
from app.helpers.utils.general.name_generation import generate_name

from tests.constants import REGION_NAMES, CLIENT_COMMIT_HASH_FOR_TESTING, ACTIVE_AMI, INACTIVE_AMIS


def _generate_region_to_ami_objs(ami_id, client_commit_hash, is_ami_active):
    """
    This function generates RegionToAmi objects with the supplied <ami_id>, <client_commit_hash>,
    <ami_active> for all the regions.
    Args:
        ami_id: AMI id for the region to ami row.
        client_commit_hash: client commit hash that is compatible with the ami_id
        ami_active: True if the next instance that is launched should be with this AMI.
    Returns:

    """
    region_ami_objs = []
    for region_name in REGION_NAMES:
        region_ami_objs.append(
            RegionToAmi(
                region_name=region_name,
                ami_id=ami_id,
                client_commit_hash=client_commit_hash,
                ami_active=is_ami_active,
            )
        )
    return region_ami_objs


def populate_test_data():
    """
    This function generates RegionToAmi objects across all regions with active and inactive client hashes
    and populates them into the database.
    """
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
