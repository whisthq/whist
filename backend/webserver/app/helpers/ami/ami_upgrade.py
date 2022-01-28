from threading import Thread
from typing import cast, Any, List, Mapping, Tuple
from flask import current_app, Flask
from sqlalchemy import or_, and_

from app.database.models.cloud import db, RegionToAmi, InstanceInfo, MandelboxHostState
from app.utils.general.logs import whist_logger
from app.helpers.aws.aws_instance_post import (
    do_scale_up_if_necessary,
    drain_instance,
)
from app.helpers.aws.aws_instance_state import _poll

#  This list allows thread success to be passed back to the main thread.
#  It is thread-safe because lists in Python are thread-safe.
#  It is a list of lists, with each sublist containing a thread, a
#  boolean demarcating that thread's status (i.e. whether it succeeded), a
#  tuple of (region_name, ami_id) associated with that thread, and the name of the
#  created instance.
region_wise_upgrade_threads: List[List[Any]] = []


def insert_new_amis(
    client_commit_hash: str, region_to_ami_id_mapping: Mapping[str, str]
) -> List[RegionToAmi]:
    """
    Inserts new AMIs (but doesn't mark them as active) into the RegionToAmi table.
    Will be invoked from the Flask CLI command to upgrade the AMIs for regions.
    We should mark the new AMIs as active only after we cleanup the existing instances and AMIs.
    If an AMI is already present, returns its row.
    Args:
        client_commit_hash: Commit hash of the client that is compatible with the AMIs
                            that are going to be passed in as the other argument.
        region_to_ami_id_mapping: Dict<Region, AMI> Dict of regions to AMIs that are compatible
                                    with the <client_commit_hash>.

    Returns:
        A list of the regionToAmi objects for this set of regions and commit hashes.
    """
    new_amis = []
    for region_name, ami_id in region_to_ami_id_mapping.items():
        new_ami = RegionToAmi(
            region_name=region_name,
            ami_id=ami_id,
            client_commit_hash=client_commit_hash,
            ami_active=False,
        )
        new_amis.append(new_ami)
        db.session.merge(new_ami)
    db.session.commit()
    return new_amis


def launch_new_ami_buffer(
    region_name: str, ami_id: str, index_in_thread_list: int, flask_app: Flask
) -> None:
    """
    This function will be invoked from the Flask CLI command to upgrade the AMIs for regions.
    Inserts new AMIs into the RegionToAmi table and blocks until the instance is started, and the
    host service is active and has marked the instance as active in the database.

    Args:
        region_name: Name of the region in which new instances neeed to be launched.
        ami_id: AMI for the instances that need to be launched.
        index_in_thread_list:  which bool in the thread list to set to true for exception tracking
        flask_app: Needed for providing the context need for interacting with the database.

    Returns:
        None.
    """
    global region_wise_upgrade_threads  # pylint:disable=global-variable-not-assigned,invalid-name
    whist_logger.debug(f"launching_instances in {region_name} with ami: {ami_id}")
    with flask_app.app_context():
        force_buffer = flask_app.config["DEFAULT_INSTANCE_BUFFER"]
        new_instance_names = do_scale_up_if_necessary(
            region_name, ami_id, force_buffer, flask_app=flask_app
        )
        result = False  # Make Pyright stop complaining
        assert len(new_instance_names) > 0  # This should always hold
        for new_instance_name in new_instance_names:
            whist_logger.debug(
                f"Waiting for instance with name: {new_instance_name} to be marked online"
            )
            result = _poll(new_instance_name)
            if result:
                region_wise_upgrade_threads[index_in_thread_list][3] = new_instance_name
    region_wise_upgrade_threads[index_in_thread_list][1] = result


def fetch_current_running_instances(amis_to_exclude: List[str]) -> List[InstanceInfo]:
    """
    Fetches the instances that are either
        ACTIVE - Instances that were launched and have potentially users running their applications
            on it.
        PRE_CONNECTION - Instances that are launched few instants ago but haven't
                        marked themselves active in the database through host service.
    Args:
        amis_to_exclude -> List of new AMIs from this upgrade. We will be using this list to
            differentiate between the instances launched from new AMIs that we are going to upgrade
            to and the current/older AMIs. We can just mark all the running instances as DRAINING
            before we start the instances with new AMI but that is going to increase the length of
            our unavailable window for users. This reduces the window by the time it takes to spin
            up and warm up an AWS instance, which can be several minutes.
    Returns:
        List[InstanceInfo] -> List of instances that are currently running.
    """
    return (  # type: ignore[no-any-return]
        db.session.query(InstanceInfo)
        .filter(
            and_(
                or_(
                    InstanceInfo.status.like(MandelboxHostState.ACTIVE),
                    InstanceInfo.status.like(MandelboxHostState.PRE_CONNECTION),
                ),
                InstanceInfo.aws_ami_id.not_in(amis_to_exclude),
            )
        )
        .with_for_update()
        .all()
    )


def create_ami_buffer(
    client_commit_hash: str, region_to_ami_id_mapping: Mapping[str, str]
) -> Tuple[List[str], bool]:
    """
    Creates new instances for the AMIs in the regions that are passed in as the keys of the
    region_to_ami_id_mapping

    This happens in the following steps:
        - Get current active AMIs in the database, these will be marked as inactive once
          we have sufficient buffer capacity from the new AMIs
        - Insert the new AMIs that are passed in as an argument to this function and associate them
          with the client_commit_hash.
        - Launch new instances in regions with the new AMIs and wait until they are up and mark
          themselves as active in the instances table. Since this launching the instances is going
          to take time, we will be using a thread per each region to parallelize the process.

    Edge cases:
        - What if the argument region_to_ami_id_mapping has more regions than we currently support.
          This should be fine as we use the regions passed in as the argument as a reference to
          spin up instances in new regions.
        - What if the argument region_to_ami_id_mapping has less number of regions than we
          currently support. Then, we would update the AMIs in the regions that are passed in as
          the argument and mark the AMIs in missing regions as disabled. Also, we would be marking
          all the current running instances for draining. So, this would essentially purge the
          regions that are missing in the passed in arguments.

    Args:
        client_commit_hash: Commit hash of the client that is compatible with the AMIs that are
            going to be passed in as the other argument.
        region_to_ami_id_mapping: String representation of Dict<Region, AMI>. Dict of regions to
            AMIs that are compatible with the <client_commit_hash>.

    Returns:
        None
    """
    global region_wise_upgrade_threads  # pylint: disable=global-variable-not-assigned,invalid-name

    new_amis = insert_new_amis(client_commit_hash, region_to_ami_id_mapping)
    new_amis_str = [
        new_ami.ami_id for new_ami in new_amis
    ]  # Fetching the AMI strings for instances running with new AMIs.
    # This will be used to select only the instances with current/older AMIs

    for ami in new_amis:
        region_name = ami.region_name
        ami_id = ami.ami_id
        # grab a lock here
        region_row = (
            RegionToAmi.query.filter_by(region_name=region_name, ami_id=ami_id)
            .with_for_update()
            .one_or_none()
        )
        if region_row is not None:
            region_row.protected_from_scale_down = True
        db.session.commit()
        region_wise_upgrade_thread = Thread(
            target=launch_new_ami_buffer,
            args=(region_name, ami_id, len(region_wise_upgrade_threads)),
            # current_app is a proxy for app object, so `_get_current_object` method
            # should be used to fetch the application object to be passed to the thread.
            kwargs={
                "flask_app": current_app._get_current_object()  # pylint: disable=protected-access
            },
        )
        region_wise_upgrade_threads.append(
            [region_wise_upgrade_thread, False, (region_name, ami_id), None]
        )
        region_wise_upgrade_thread.start()
    threads_succeeded = True
    amis_failed = False
    instances_created = []
    regions_failed = []
    for region_and_bool_pair in region_wise_upgrade_threads:
        region_and_bool_pair[0].join()
        if not region_and_bool_pair[1]:
            region_name, ami_id = region_and_bool_pair[2]
            regions_failed.append(region_name)
            region_row = (
                RegionToAmi.query.filter_by(region_name=region_name, ami_id=ami_id)
                .with_for_update()
                .one_or_none()
            )
            if region_row is not None:
                region_row.protected_from_scale_down = False
            db.session.commit()
            threads_succeeded = False
            amis_failed = True
        else:
            instances_created.append(region_and_bool_pair[3])
    if not threads_succeeded:
        whist_logger.info(f"Some regions failed to upgrade: {regions_failed}.")
        whist_logger.info(f"draining all newly active instances: {instances_created}")
        for instance in instances_created:
            instance = cast(str, instance)
            instanceinfo = InstanceInfo.query.get(instance)
            drain_instance(instanceinfo)
        # If any thread here failed, fail the workflow
        raise Exception("AMIS failed to upgrade, see logs")
    return new_amis_str, amis_failed


def swapover_amis(new_amis_str: List[str], amis_failed: bool) -> None:
    """
    Actually swaps over which AMIs are active, once a new buffer is spun up. The swapover only
    happens if there were no errors starting buffers for the new AMIs. That is, if even one fails
    to create a buffer, the whole swapover operation will fail and the new AMIs will be removed from
    the database, effectively making the upgrade flow atomic.

    STEPS:
        1) Drains all instances of old AMIs
        2) Sets the old AMIs to inactive and the new ones to active.
    Args:
        new_amis: The new AMI rows we want to be active.
        amis_failed: Indicates if any ami failed to create a buffer.

    Returns:
        None
    """

    # If amis_failed is true, it means at least one region failed to upgrade. Only swapover if
    # all of the region buffers were created successfully.

    if not amis_failed:
        new_amis = [RegionToAmi.query.filter_by(ami_id=ami_id).first() for ami_id in new_amis_str]
        region_current_active_ami_map = {}
        current_active_amis = RegionToAmi.query.filter_by(ami_active=True).all()
        for current_active_ami in current_active_amis:
            region_current_active_ami_map[current_active_ami.region_name] = current_active_ami

        for new_ami in new_amis:
            new_ami.protected_from_scale_down = False
            new_ami.ami_active = True

        for current_ami in current_active_amis:
            current_ami.ami_active = False

        db.session.commit()

        current_running_instances = fetch_current_running_instances(new_amis_str)
        for active_instance in current_running_instances:
            drain_instance(active_instance)

        whist_logger.info("Finished performing AMI upgrade.")
    else:
        whist_logger.info(
            "Failed to create buffer for some AMIs, so not performing swapover operation. Rolling "
            "back new AMIs from database."
        )

        # Rollback new AMIs from the database. Only delete the new ami rows
        # since they were not marked as active yet.
        for ami_id in new_amis_str:
            RegionToAmi.query.filter_by(ami_id=ami_id).delete()

        db.session.commit()
