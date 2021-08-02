import sys
from threading import Thread
from typing import cast, Any, List, Mapping
import requests
from flask import current_app
from sqlalchemy import or_, and_

from app.models import db, RegionToAmi, InstanceInfo
from app.helpers.utils.auth0 import Auth0Client
from app.helpers.utils.general.logs import fractal_logger
from app.helpers.blueprint_helpers.aws.aws_instance_post import (
    do_scale_up_if_necessary,
    terminate_instance,
)
from app.helpers.blueprint_helpers.aws.aws_instance_state import _poll
from app.constants.instance_state_values import InstanceState

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
    Args:
        client_commit_hash: Commit hash of the client that is compatible with the AMIs
                            that are going to be passed in as the other argument.
        region_to_ami_id_mapping: Dict<Region, AMI> Dict of regions to AMIs that are compatible
                                    with the <client_commit_hash>.

    Returns:
        A list of the created RegionToAmi objects that are created.
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
    db.session.add_all(new_amis)
    db.session.commit()
    return new_amis


def launch_new_ami_buffer(
    region_name: str, ami_id: str, index_in_thread_list: int, flask_app
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
    global region_wise_upgrade_threads
    fractal_logger.debug(f"launching_instances in {region_name} with ami: {ami_id}")
    with flask_app.app_context():
        force_buffer = flask_app.config["DEFAULT_INSTANCE_BUFFER"]
        new_instance_names = do_scale_up_if_necessary(
            region_name, ami_id, force_buffer, flask_app=flask_app
        )
        result = False  # Make Pyright stop complaining
        assert len(new_instance_names) > 0  # This should always hold
        for new_instance_name in new_instance_names:
            fractal_logger.debug(
                f"Waiting for instance with name: {new_instance_name} to be marked online"
            )
            result = _poll(new_instance_name)
            if result:
                region_wise_upgrade_threads[index_in_thread_list][3] = new_instance_name
    region_wise_upgrade_threads[index_in_thread_list][1] = result


def mark_instance_for_draining(active_instance: InstanceInfo) -> bool:
    """
    Marks the instance for draining by calling the drain_and_shutdown endpoint of the host service
    and marks the instance as draining. If the endpoint errors out with an unexpected status code,
    we are going to mark the instance as unresponsive. We are not going to kill the instance as at
    this point in time, we won't be sure if all the users have left the instance. Once the instance
    is marked as draining, we won't launch associate a "mandelbox" running on this instance to an user.

    Note that we shouldn't call this function on a single instance multiple times. In particular,
    the http server in a host service is shut down soon after the endpoint is called, which means
    that future requests are always going to fail, unfairly marking the host service as
    unresponsive.

    Args:
        active_instance: InstanceInfo object for the instance that need to be marked as draining.
    Returns:
        job_status: A boolean indicating if we are able to mark the instance as draining by
        calling the drain_and_shutdown endpoint on host service.
    """
    job_status = False
    fractal_logger.info(
        f"mark_instance_for_draining called for instance {active_instance.instance_name}"
    )
    if str(active_instance.ip) == "":
        try:
            terminate_instance(active_instance)
            job_status = True
        except:
            job_status = False
    try:
        auth0_client = Auth0Client(
            current_app.config["AUTH0_DOMAIN"],
            current_app.config["AUTH0_WEBSERVER_CLIENT_ID"],
            current_app.config["AUTH0_WEBSERVER_CLIENT_SECRET"],
        )
        auth_token = auth0_client.token().access_token
        base_url = f"https://{active_instance.ip}:{current_app.config['HOST_SERVICE_PORT']}"
        resp = requests.post(
            f"{base_url}/drain_and_shutdown",
            json={
                "auth_secret": auth_token,
            },
            verify=False,  # SSL verification turned off due to self signed certs on host service.
        )
        resp.raise_for_status()
        # Host service would be setting the state in the DB once we call the drain endpoint.
        # However, there is no downside to us setting this as well.
        active_instance.status = InstanceState.DRAINING
        fractal_logger.info(
            f"mark_instance_for_draining successfully sent POST to instance {active_instance.instance_name}"
        )
        job_status = True
    except requests.exceptions.RequestException as error:
        active_instance.status = InstanceState.HOST_SERVICE_UNRESPONSIVE
        fractal_logger.error(
            f"mark_instance_for_draining failed to send POST to instance {active_instance.instance_name}: {error}"
        )
    finally:
        db.session.commit()
    return job_status


def fetch_current_running_instances(amis_to_exclude: List[str]) -> List[InstanceInfo]:
    """
    Fetches the instances that are either
        ACTIVE - Instances that were launched and have potentially users running their applications on it.
        PRE_CONNECTION - Instances that are launched few instants ago but haven't
                        marked themselves active in the database through host service.
    Args:
        amis_to_exclude -> List of new AMIs from this upgrade. We will be using this list to differentiate
        between the instances launched from new AMIs that we are going to upgrade to and the current/older AMIs.
        We can just mark all the running instances as DRAINING before we start the instances with new AMI but that
        is going to increase the length of our unavailable window for users. This reduces the window by the time
        it takes to spin up and warm up an AWS instance, which can be several minutes.
    Returns:
        List[InstanceInfo] -> List of instances that are currently running.
    """
    return (  # type: ignore[no-any-return]
        db.session.query(InstanceInfo)
        .filter(
            and_(
                or_(
                    InstanceInfo.status.like(InstanceState.ACTIVE),
                    InstanceInfo.status.like(InstanceState.PRE_CONNECTION),
                ),
                InstanceInfo.aws_ami_id.not_in(amis_to_exclude),
            )
        )
        .with_for_update()
        .all()
    )


def perform_upgrade(client_commit_hash: str, region_to_ami_id_mapping: Mapping[str, str]) -> None:
    """
    Performs upgrade of the AMIs in the regions that are passed in as the keys of the region_to_ami_id_mapping
    This happens in the following steps:
        - Get current active AMIs in the database, these will be marked as inactive once
        we have sufficient buffer capacity from the new AMIs
        - Insert the new AMIs that are passed in as an argument to this function and associate them with the
        client_commit_hash.
        - Launch new instances in regions with the new AMIs and wait until they are up and mark themselves as
        active in the instances table. Since this launching the instances is going to take time, we will be using
        a thread per each region to parallelize the process.
        - Once the instances with the new AMIs are up and running, we will mark the instances that are running with
        an older AMI version (i.e the current active AMI versions) as DRAINING to stop associating users with mandelboxes
        on the instances.
        - Once all the instances across all the regions are up, mark the current active AMIs as inactive and the
        new AMIs as active.

        Edge cases:
            - What if the argument region_to_ami_id_mapping has more regions than we currently support. This should be fine
            as we use the regions passed in as the argument as a reference to spin up instances in new regions.
            - What if the argument region_to_ami_id_mapping has less number of regions than we currently support. Then, we
            would update the AMIs in the regions that are passed in as the argument and mark the AMIs in missing regions as disabled.
            Also, we would be marking all the current running instances for draining. So, this would essentially purge the regions
            that are missing in the passed in arguments.

        Args:
            client_commit_hash: Commit hash of the client that is compatible with the AMIs
                            that are going to be passed in as the other argument.
            region_to_ami_id_mapping: String representation of Dict<Region, AMI>. Dict of regions to AMIs that are compatible
                                        with the <client_commit_hash>.

        Returns:
            None
    """
    global region_wise_upgrade_threads
    region_current_active_ami_map = {}
    current_active_amis = RegionToAmi.query.filter_by(ami_active=True).all()
    for current_active_ami in current_active_amis:
        region_current_active_ami_map[current_active_ami.region_name] = current_active_ami

    new_amis = insert_new_amis(client_commit_hash, region_to_ami_id_mapping)

    for region_name, ami_id in region_to_ami_id_mapping.items():
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
            kwargs={"flask_app": current_app._get_current_object()},
        )
        region_wise_upgrade_threads.append(
            [region_wise_upgrade_thread, False, (region_name, ami_id), None]
        )
        region_wise_upgrade_thread.start()
    threads_succeeded = True
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
        else:
            instances_created.append(region_and_bool_pair[3])
    if not threads_succeeded:
        fractal_logger.info(f"Some regions failed to upgrade: {regions_failed}.")
        fractal_logger.info(f"draining all newly active instances: {instances_created}")
        for instance in instances_created:
            instance = cast(str, instance)
            instanceinfo = InstanceInfo.query.get(instance)
            mark_instance_for_draining(instanceinfo)
        # If any thread here failed, fail the workflow
        raise Exception("AMIS failed to upgrade, see logs")

    new_amis_str = [
        new_ami.ami_id for new_ami in new_amis
    ]  # Fetching the AMI strings for instances running with new AMIs.
    # This will be used to select only the instances with current/older AMIs

    current_running_instances = fetch_current_running_instances(new_amis_str)
    for active_instance in current_running_instances:
        # At this point, we should still have the lock that we grabbed when we
        # invoked the `fetch_current_running_instances` function. Using this
        # lock, we mark the instances as DRAINING to prevent a mandelbox from
        # being assigned to the instances.
        fractal_logger.info(f"Draining instance {active_instance.instance_name} in database only!")
        active_instance.status = InstanceState.DRAINING
    db.session.commit()

    mark_instance_for_draining_failures = []
    for active_instance in current_running_instances:
        # At this point, the instance is marked as DRAINING in the database. But we need
        # to inform the HOST_SERVICE that we have marked the instance as draining.
        active_instance_draining_status = mark_instance_for_draining(active_instance)
        if active_instance_draining_status is False:
            # Mark the status for draining all instances as False when marking any instance draining fails.
            fractal_logger.error(
                f"Failed to mark instance: {active_instance.instance_name} for draining through host service."
            )
            mark_instance_for_draining_failures.append(active_instance.instance_name)

    for new_ami in new_amis:
        new_ami.protected_from_scale_down = False
        new_ami.ami_active = True

    for current_ami in current_active_amis:
        current_ami.ami_active = False

    # Reset the list here to ensure no thread status info leaks
    region_wise_upgrade_threads = []
    db.session.commit()

    fractal_logger.info("Finished performing AMI upgrade.")
    if len(mark_instance_for_draining_failures) > 0 and not current_app.testing:
        for failed_instance in mark_instance_for_draining_failures:
            fractal_logger.error(
                f"Failed to mark instance: {failed_instance} as draining through host service"
            )
        sys.exit(1)
