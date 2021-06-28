import os
import time

from typing import Any, Dict, List, Optional, Tuple

import requests

from celery import shared_task, current_task
from flask import current_app
from requests import ConnectionError, Timeout, TooManyRedirects

from app.maintenance.maintenance_manager import maintenance_track_task
from app.celery.aws_ecs_deletion import delete_cluster
from app.celery.aws_ecs_modification import manual_scale_cluster, check_and_cleanup_outdated_tasks
from app.helpers.utils.aws.base_ecs_client import ECSClient
from app.helpers.utils.general.logs import fractal_logger
from app.helpers.utils.general.sql_commands import fractal_sql_commit
from app.helpers.utils.general.sql_commands import fractal_sql_update
from app.helpers.utils.aws.aws_resource_integrity import ensure_container_exists
from app.helpers.utils.db.db_utils import set_local_lock_timeout
from app.helpers.utils.auth0.auth0 import Auth0Client

from app.models import (
    db,
    UserContainer,
    ClusterInfo,
    SortedClusters,
    SupportedAppImages,
    RegionToAmi,
)
from app.helpers.blueprint_helpers.aws.container_state import set_container_state
from app.helpers.utils.event_logging.events import (
    logged_event_container_assigned,
    logged_event_container_prewarmed,
    logged_event_cluster_created,
)

from app.serializers.hardware import UserContainerSchema, ClusterInfoSchema

from app.constants.container_state_values import (
    FAILURE,
    PENDING,
    READY,
    SPINNING_UP_NEW,
    WAITING_FOR_CLIENT_APP,
)

from app.celery.aws_celery_exceptions import ContainerNotAvailableError

from app.exceptions import StartValueException


MAX_PASS_START_VALUES_RETRIES = 3
MAX_POLL_ITERATIONS = 90
user_container_schema = UserContainerSchema()
user_cluster_schema = ClusterInfoSchema()


def _clean_up_task(container: UserContainer) -> None:
    """
    Stops a running task and deletes it from the DB
    Args:
        container: the userContainer object to stop

    Returns: None

    """
    fractal_logger.info(f"cleaning up container {container.container_id}")
    # stop base container task if it is running
    ecs_client = ECSClient(
        base_cluster=container.cluster, region_name=container.location, grab_logs=False
    )
    ecs_client.add_task(container.container_id)

    if not ecs_client.check_if_done(offset=0):
        ecs_client.stop_task(reason="Container failed to properly initialize", offset=0)
    # delete base container from db
    fractal_sql_commit(db, lambda db, x: db.session.delete(x), container)


def _clean_tasks_and_create_new_container(
    container: UserContainer,
    task_version: int,
    webserver_url: str,
    num_tries: int,
) -> Dict[str, Any]:
    """
    This function is called by _assign_container() upon failure of _pass_start_values(). It cleans
    up resources associated with the container that failed (stops and delete tasks), then retries
    _assign_container().

    Arguments:
        container: An instance of the UserContainer model.
        webserver_url: the webserver originating the initial assign_container request
        num_tries: the current number of attempts to call _pass_start_values.

    Returns:
        After cleaning resources, returns the result of recursive call to assign_container,
        which is a generated container, in json form
    """
    _clean_up_task(container)

    # delete every task that is unassigned with that instance IP in the DB
    all_tasks = UserContainer.query.filter_by(ip=container.ip, user_id=None).all()
    for task in all_tasks:
        _clean_up_task(task)

    # assign a new container for that user
    return assign_container(
        container.user_id,
        container.task_definition,
        task_version,
        container.location,
        container.cluster,
        container.dpi,
        webserver_url,
        num_tries,
    )


def _pass_start_values_to_instance(
    container: UserContainer, client_app_auth_secret: str = ""
) -> None:
    """
    Send the instance start values to the host service.

    Arguments:
        container: An instance of the UserContainer model.
         client_app_auth_secret: the auth secret the client app should use

    Returns:
        None

    Raises:
        StartValueException: When fails to connect to ECS host service or
            when set-start-values generates a bad response
    """

    try:
        auth0_client = Auth0Client(
            current_app.config["AUTH0_DOMAIN"],
            current_app.config["AUTH0_WEBSERVER_CLIENT_ID"],
            current_app.config["AUTH0_WEBSERVER_CLIENT_SECRET"],
        )
        auth_token = auth0_client.token().access_token
        response = requests.put(
            (
                f"https://{container.ip}:{current_app.config['HOST_SERVICE_PORT']}"
                "/set_container_start_values"
            ),
            json={
                "host_port": container.port_32262,
                "container_ARN": container.container_id,
                "dpi": container.dpi,
                "user_id": container.user_id,
                "client_app_auth_secret": client_app_auth_secret,
                "auth_secret": auth_token,
            },
            verify=False,
        )
    except (ConnectionError, Timeout, TooManyRedirects) as error:
        fractal_logger.error(
            (
                "Encountered an error while attempting to connect to the ECS host service running "
                f"on {container.ip}: {error}"
            ),
        )
        raise StartValueException from error
    else:
        if response.ok:
            fractal_logger.info("Container user values set.")
        else:
            fractal_logger.error(
                f"Received unsuccessful set-start-values response: {response.text}"
            )
            raise StartValueException


bundled_region = {
    "us-east-1": ["us-east-2"],
    "us-east-2": ["us-east-1"],
    "us-west-1": ["us-west-2"],
    "us-west-2": ["us-west-1"],
}


def find_available_container(
    region_name: str, task_definition_arn: str, task_version: Optional[int] = None
) -> Optional[UserContainer]:
    """
    Function to find an unassigned container with the right taskdef
    note that now UID=None means that a container is unassigned
    Args:
        region_name: which region to look for
        task_definition_arn: which taskdef to use
        task_version: which version of the task we need to find

    Returns: either a fitting container or None if no container is found

    """
    # 30sec arbitrarily decided as sufficient timeout when using with_for_update
    set_local_lock_timeout(30)
    available_container = (
        UserContainer.query.filter_by(
            task_definition=task_definition_arn,
            location=region_name,
            user_id=None,
            task_version=task_version,
        )
        .filter(UserContainer.cluster.notlike("%test%"))
        .with_for_update()
        .limit(1)
        .one_or_none()
    )

    if available_container is None:
        # check each replacement region for available containers
        for bundlable_region in bundled_region.get(region_name, []):
            # 30sec arbitrarily decided as sufficient timeout when using with_for_update
            set_local_lock_timeout(30)
            available_container = (
                UserContainer.query.filter_by(
                    task_definition=task_definition_arn,
                    location=bundlable_region,
                    user_id=None,
                    task_version=task_version,
                )
                .filter(UserContainer.cluster.notlike("%test%"))
                .with_for_update()
                .limit(1)
                .one_or_none()
            )
            if available_container is not None:
                break
    return available_container


def _poll(container_id: str) -> bool:
    """Poll the database until the web server receives its first ping from the new container.

    Time out after MAX_POLL_ITERATIONS seconds.
    This may be an appropriate use case for Hasura subscriptions.

    This function should patched to immediately return True in order to get CI to pass.

    Arguments:
        container_id: The container ID of the container whose state to poll.

    Returns:
        True if and only if the container's starts with RUNNING_ by the end of the polling period.
    """

    container = UserContainer.query.get(container_id)
    result = False

    for i in range(MAX_POLL_ITERATIONS):
        if not container.state.startswith("RUNNING_"):
            fractal_logger.warning(
                f"{container.container_id} deployment in progress. {i}/{MAX_POLL_ITERATIONS}"
            )
            time.sleep(1)
            db.session.refresh(container)
        else:
            result = True
            break

    return result


def select_cluster(region_name: str) -> str:
    """
    Selects the best cluster for task placement in a given region, creating new clusters as
    necessary.
    Args:
        region_name: which region to create the cluster in.

    Returns: a valid cluster name.

    """

    all_clusters = list(SortedClusters.query.filter_by(location=region_name).all())
    all_clusters = [cluster for cluster in all_clusters if "cluster" in cluster.cluster]
    base_len = 2
    regen_fraction = 1.0
    if len(all_clusters) == 0:
        fractal_logger.info("No available clusters found. Creating new cluster...")
        cluster_name = create_new_cluster.delay(region_name=region_name, ami=None, min_size=1).get(
            disable_sync_subtasks=False
        )["cluster"]
        for _ in range(base_len - len(all_clusters)):
            create_new_cluster.delay(region_name=region_name, ami=None, min_size=1)
    else:
        cluster_name = all_clusters[0].cluster
        if len(all_clusters) == 1 and float(
            all_clusters[0].registeredContainerInstancesCount
        ) >= regen_fraction * float(all_clusters[0].maxContainers):
            create_new_cluster.delay(region_name=region_name, ami=None, min_size=1)
        # delete spurious clusters
        if len(all_clusters) > 2:
            for cluster in all_clusters[2:]:
                if cluster.pendingTasksCount + cluster.runningTasksCount == 0:
                    delete_cluster.delay(cluster, force=True)
    return cluster_name


def start_container(
    webserver_url: str,
    region_name: str,
    cluster_name: str,
    task_definition_arn: str,
    task_version: Optional[int] = None,
) -> Tuple[str, int, Dict[int, int], str]:
    """
    This helper function configures and starts a container running

    Args:
        webserver_url: the URL of the webserver the container should connect to
        region_name: which region to run the container in
        cluster_name: which cluster to run the container in
        task_definition_arn: which taskdef to use
        task_version: which taskdef version to use. If None, uses the latest task version.

    Returns: the task_id, IP, port bindings, and aeskey of the container once running

    """
    aeskey = os.urandom(16).hex()
    container_overrides = {
        "containerOverrides": [
            {
                "name": "fractal-container",
                "environment": [
                    {"name": "FRACTAL_AES_KEY", "value": aeskey},
                    {
                        "name": "WEBSERVER_URL",
                        "value": (webserver_url if webserver_url is not None else ""),
                    },
                ],
            },
        ],
    }
    kwargs = {"networkConfiguration": None, "overrides": container_overrides}

    ecs_client = ECSClient(launch_type="EC2", region_name=region_name)

    ecs_client.set_cluster(cluster_name)
    if task_version is None:
        # if task_version is None, we just pass the task_definition_arn
        # which AWS interprets to mean run the latest version.
        # this solves backcompat to pre-version pinning days
        fractal_logger.warning(f"Please pin the version for task {task_definition_arn}")
        ecs_client.set_task_definition_arn(task_definition_arn)
    else:
        # concatenating in this format runs the specific version of the task def
        ecs_client.set_task_definition_arn(f"{task_definition_arn}:{task_version}")
    ecs_client.run_task(False, **{k: v for k, v in kwargs.items() if v is not None})

    # 2 * 300 = 600 secs = 10 minutes max, but with AWS API latencies more like 11-12
    if not ecs_client.spin_til_running(time_delay=2, max_polls=300):
        aws_task_arn = ecs_client.tasks[0]
        raise ContainerNotAvailableError(aws_task_arn, 300)

    curr_ip = ecs_client.task_ips.get(0, -1)
    curr_network_binding = ecs_client.task_ports.get(0, -1)
    task_id = ecs_client.tasks[0]
    return task_id, curr_ip, curr_network_binding, aeskey


def _get_num_extra(taskdef: str, location: str) -> int:
    """
    Function determining how many containers to preboot based on type

    Specifically, given a taskdef and region, we first figure out
    how many tasks we want as a buffer -- which is proportional
    to the number of live users.  We then see how full our
    current buffer is, and return the amount we'd need to
    expand it to get to our ideal buffer

    :param taskdef: the task definition ARN of the container
    :param location:  what region the task is being booted in
    :return: integer determining how many containers to preboot
    """

    def _get_count_helper(query):
        # based on https://gist.github.com/hest/8798884
        from sqlalchemy.sql.functions import func

        count_q = query.statement.with_only_columns([func.count()]).order_by(None)
        count = query.session.execute(count_q).scalar()
        return count

    app_image_for_taskdef = SupportedAppImages.query.filter_by(task_definition=taskdef).first()
    if app_image_for_taskdef:
        # get the number we want prewarmed
        num_needed_running = float(
            _get_count_helper(
                UserContainer.query.filter(
                    UserContainer.task_definition == taskdef,
                    UserContainer.location == location,
                    # SQLAlchemy does not correctly use the "is not None" semantic
                    UserContainer.user_id != None,  # pylint: disable=singleton-comparison
                )
            )
        )
        # then floor it at 1
        num_needed_running = max(1, num_needed_running * app_image_for_taskdef.preboot_number)
        # then see how many prewarmed are currently running
        num_currently_running = _get_count_helper(
            UserContainer.query.filter_by(task_definition=taskdef, location=location, user_id=None)
        )
        return max(0, int(num_needed_running - num_currently_running))
    return 0


@shared_task
@maintenance_track_task
def assign_container(
    username: str,
    task_definition_arn: str,
    task_version: Optional[int] = None,
    region_name: str = "us-east-1",
    cluster_name: Optional[str] = None,
    dpi: Optional[int] = 96,
    webserver_url: str = "fractal-dev-server.herokuapp.com",
    num_tries: Optional[int] = 0,
) -> Dict[str, Any]:
    """
    Assigns a running container to a user, or creates one if none exists

    :param username: the username of the requesting user
    :param task_definition_arn: which taskdef the user needs a container for
    :param task_version: the version of the taskdef to use. If None, uses latest in db.
    :param region_name: which region the user needs a container for
    :param cluster_name: which cluster the user needs a container for, only used in test
    :param dpi: the user's DPI
    :param webserver_url: the webserver originating the request
    :param num_tries: the current number of attempts to call _pass_start_values
    :return: the generated container, in json form

    We directly call _assign_container because it can easily be mocked. The __code__ attribute
    does not exist for functions with celery decorators like this one.
    """
    return _assign_container(
        username,
        task_definition_arn,
        task_version,
        region_name,
        cluster_name,
        dpi,
        webserver_url,
        num_tries,
    )


def _assign_container(
    username: str,
    task_definition_arn: str,
    task_version: Optional[int] = None,
    region_name: str = "us-east-1",
    cluster_name: Optional[str] = None,
    dpi: Optional[int] = 96,
    webserver_url: str = "fractal-dev-server.herokuapp.com",
    num_tries: Optional[int] = 0,
) -> Dict[str, Any]:
    """
    See assign_container. This is helpful to mock.
    """
    fractal_logger.info(f"Starting to assign container in {region_name}", extra={"label": username})

    set_container_state(
        keyuser=username,
        keytask=current_task.request.id,
        task_id=current_task.request.id,
        state=PENDING,
        force=True,  # necessary since check will fail otherwise
    )

    client_app_auth_secret = os.urandom(16).hex()
    enable_reconnect = False
    task_start_time = time.time()

    existing_container: Optional[UserContainer] = None

    # if a cluster is passed in, we're in testing mode:
    if cluster_name is None:
        if enable_reconnect:
            # first, we check for a preexisting container with the correct user:
            try:
                existing_container = ensure_container_exists(
                    UserContainer.query.filter_by(
                        user_id=username,
                        task_definition=task_definition_arn,
                        task_version=task_version,
                        location=region_name,
                    )
                    .limit(1)
                    .first()
                )
            except Exception:
                # If the `filter_by` gave us `None`, that is okay, we handle that below.
                # Simply set the output appropriately.
                existing_container = None

            if existing_container:
                if _poll(existing_container.container_id):
                    return user_container_schema.dump(existing_container)

        # otherwise, we see if there's an unassigned container
        base_container: Optional[UserContainer] = None

        try:
            base_container = ensure_container_exists(
                find_available_container(region_name, task_definition_arn, task_version)
            )
        except Exception:
            # If the `filter_by` gave us `None`, that is okay, we handle that below.
            # For now, we end the session so that code from here runs in a new transaction
            # This is necessary because if a lock timeout brought us here AND we do
            # not end the session, then any future SQL commands run in the same bad
            # erroring transaction and are invalidated. Basically, the rest of this
            # function can never touch the db unless we end the session here.
            db.session.close()

    else:
        num_extra = 0
        base_container = None
    if base_container is not None:
        base_container.user_id = username
        base_container.dpi = dpi
        db.session.commit()
        current_task.update_state(
            state="PENDING",
            meta={"msg": "App assigned"},
        )
    else:
        set_container_state(
            keyuser=username,
            keytask=current_task.request.id,
            task_id=current_task.request.id,
            state=SPINNING_UP_NEW,
            force=True,  # necessary since check will fail otherwise
        )
        current_task.update_state(
            state="PENDING",
            meta={"msg": "No waiting app found -- creating a new one"},
        )
        if cluster_name is None:
            cluster_name = select_cluster(region_name)
        fractal_logger.info(
            f"Creating new container in cluster {cluster_name}", extra={"label": username}
        )

        cluster_info = ClusterInfo.query.filter_by(cluster=cluster_name).first()

        if cluster_info.status == "DEPROVISIONING":
            set_container_state(
                keyuser=username,
                keytask=current_task.request.id,
                task_id=current_task.request.id,
                state=FAILURE,
            )
            fractal_logger.error(
                f"Cluster status is {cluster_info.status}",
                extra={
                    "label": username,
                },
            )
            raise Exception(f"Cluster {cluster_info.cluster} is {cluster_info.status}.")

        message = (
            f"Deploying {task_definition_arn}:{task_version} to {cluster_name} in {region_name}"
        )
        current_task.update_state(
            state="PENDING",
            meta={"msg": message},
        )
        fractal_logger.info(message, extra={"label": username})
        task_id, curr_ip, curr_network_binding, aeskey = start_container(
            webserver_url,
            region_name,
            cluster_name,
            task_definition_arn,
            task_version,
        )
        if curr_ip == -1 or curr_network_binding == -1:
            set_container_state(
                keyuser=username,
                keytask=current_task.request.id,
                task_id=current_task.request.id,
                state=FAILURE,
            )
            fractal_logger.error("Error generating task with running IP", extra={"label": username})
            raise Exception("Your app's network bindings were not created.")

        # TODO:  refactor this out into its own function

        ecs_client = ECSClient(launch_type="EC2", region_name=region_name)

        ecs_client.set_cluster(cluster_name)

        container = UserContainer(
            container_id=task_id,
            user_id=username,
            cluster=cluster_name,
            ip=curr_ip,
            port_32262=curr_network_binding[32262],
            port_32263=curr_network_binding[32263],
            port_32273=curr_network_binding[32273],
            state="CREATING",
            location=region_name,
            secret_key=aeskey,
            task_definition=task_definition_arn,
            task_version=task_version,
            dpi=dpi,
        )
        container_sql = fractal_sql_commit(db, lambda db, x: db.session.add(x), container)
        if container_sql:
            container = UserContainer.query.get(task_id)
            fractal_logger.info(
                (
                    f"Inserted container with IP address {curr_ip} and network bindings "
                    f"{curr_network_binding} and task ID {task_id}"
                ),
                extra={"label": username},
            )
        else:

            set_container_state(
                keyuser=username,
                keytask=current_task.request.id,
                task_id=current_task.request.id,
                state=FAILURE,
            )
            fractal_logger.info(
                f"SQL insertion of task ID {task_id} unsuccessful", extra={"label": username}
            )
            raise Exception(f"We were unable to track container {task_id} in our database.")

        cluster_usage = ecs_client.get_clusters_usage(clusters=[cluster_name])[cluster_name]
        cluster_usage["cluster"] = cluster_name
        cluster_sql = fractal_sql_commit(db, fractal_sql_update, cluster_info, cluster_usage)
        if cluster_sql:
            fractal_logger.info(
                f"Added task ID {task_id} to cluster {cluster_name} and updated cluster info"
            )
            base_container = container
        else:
            set_container_state(
                keyuser=username,
                keytask=current_task.request.id,
                task_id=current_task.request.id,
                state=FAILURE,
            )
            fractal_logger.info(
                f"SQL insertion of task ID {task_id} unsuccessful", extra={"label": username}
            )
            raise Exception(f"We were unable to assign container {task_id} to you in our database.")

    try:
        _pass_start_values_to_instance(base_container, client_app_auth_secret)
    except StartValueException:
        num_tries += 1
        if num_tries <= MAX_PASS_START_VALUES_RETRIES:
            return _clean_tasks_and_create_new_container(
                base_container,
                task_version,
                webserver_url,
                num_tries,
            )

    cluster_name = base_container.cluster

    # Now we inform the client app that the container is ready and
    # it needs to communicate with the host service to finish container startup
    # and for the container to ping the webserver
    set_container_state(
        keyuser=username,
        keytask=current_task.request.id,
        task_id=current_task.request.id,
        state=WAITING_FOR_CLIENT_APP,
        ip=base_container.ip,
        client_app_auth_secret=client_app_auth_secret,
        port=base_container.port_32262,
        force=True,  # necessary since check will fail otherwise
    )

    # Prewarm containers as/if necessary. We do this before polling below since
    # that might take a while, and for prewarming to be effective it can't wait
    # that long. We also do this before polling so that we prewarm even if
    # polling fails. This has the advantage of improving our prewarming story
    # in case a container fails to ping due to a container-specific reason
    # (i.e. not the entire host is unhealthy) and doesn't make anything worse
    # under our existing setup because we always clean up after non-pinging
    # containers.
    if not current_app.testing:
        # we figure out how empty our buffer is now
        # and then refill it
        num_extra = _get_num_extra(task_definition_arn, region_name)
        for _ in range(num_extra):
            prewarm_new_container.delay(
                task_definition_arn,
                task_version,
                region_name=region_name,
                webserver_url=webserver_url,
            )
        task_time_taken = time.time() - task_start_time
        logged_event_container_assigned(
            base_container.container_id, cluster_name, username=username, time_taken=task_time_taken
        )

    # trigger celery task to see if manual scaling should be done
    manual_scale_cluster.delay(cluster_name, region_name)

    # trigger celery task to see if task cleanup should be done
    check_and_cleanup_outdated_tasks.delay(cluster_name, region_name)

    if not _poll(base_container.container_id):
        set_container_state(
            keyuser=username,
            keytask=current_task.request.id,
            task_id=current_task.request.id,
            state=FAILURE,
        )

        fractal_logger.info(
            "container {} failed to ping".format(str(base_container.container_id)),
            extra={"label": username},
        )
        _clean_up_task(base_container)

        raise Exception(f"Container {base_container.container_id} failed to ping.")

        # pylint: disable=line-too-long
    fractal_logger.info(
        f"""container pinged!  To connect, run:
            client {base_container.ip} -p32262:{base_container.port_32262}.32263:{base_container.port_32263}.32273:{base_container.port_32273} -k {base_container.secret_key}
            """,
        extra={"label": username},
    )

    current_task.update_state(
        state="SUCCESS",
        meta=user_container_schema.dump(base_container),
    )

    set_container_state(
        keyuser=username,
        keytask=current_task.request.id,
        task_id=current_task.request.id,
        state=READY,
    )

    fractal_logger.info(
        f"""Success, task ID is {current_task.request.id} and container is ready.""",
        extra={"label": username},
    )

    return user_container_schema.dump(base_container)


# this does not need maintenance protections because it is only called by contexts that already
# have maintenance protection (assign_container, update_cluster)
@shared_task
def prewarm_new_container(
    task_definition_arn: str,
    task_version: int,
    cluster_name: Optional[str] = None,
    region_name: str = "us-east-1",
    webserver_url: str = "fractal-dev-server.herokuapp.com",
) -> Dict[str, Any]:
    """Prewarm a new ECS container running a particular task.

    Arguments:
        task_definition_arn: The task definition to use identified by its ARN.
        task_version: The version of the task def to use.
        region_name: The name of the region containing the cluster on which to
            run the container.
        cluster_name: The name of the cluster on which to run the container.
        use_launch_type: A boolean indicating whether or not to use the
            ECSClient's launch type or the cluster's default launch type.
        network_configuration: The network configuration to use for the
            clusters using awsvpc networking.
        dpi: what DPI to use on the server -- just to load into a db column
            as a placeholder since the column is nonnullable
        webserver_url: The URL of the web server to ping and with which to authenticate.
    """
    task_start_time = time.time()

    message = (
        f"Deploying {task_definition_arn}:{task_version} to "
        f"{cluster_name or 'next available cluster'} in "
        f"{region_name}"
    )

    fractal_logger.info(message)
    if not cluster_name:
        cluster_name = select_cluster(region_name)
    fractal_logger.info(
        f"Container will be deployed to cluster {cluster_name}", extra={"label": cluster_name}
    )

    cluster_info = ClusterInfo.query.filter_by(cluster=cluster_name).first()
    if not cluster_info:
        fractal_sql_commit(
            db,
            lambda db, x: db.session.add(x),
            ClusterInfo(cluster=cluster_name, location=region_name),
        )
        cluster_info = ClusterInfo.query.filter_by(cluster=cluster_name).first()

    if cluster_info.status == "DEPROVISIONING":
        fractal_logger.error(
            f"Cluster status is {cluster_info.status}", extra={"label": cluster_name}
        )
        raise Exception(f"Cluster {cluster_info.cluster} is {cluster_info.status}.")

    message = f"Deploying {task_definition_arn}:{task_version} to {cluster_name} in {region_name}"
    current_task.update_state(
        state="STARTED",
        meta={"msg": message},
    )
    fractal_logger.info(message)
    task_id, curr_ip, curr_network_binding, aeskey = start_container(
        webserver_url, region_name, cluster_name, task_definition_arn, task_version
    )
    if curr_ip == -1 or curr_network_binding == -1:
        fractal_logger.error("Error generating task with running IP", extra={"label": "prewarmed"})
        raise Exception("Your app's network bindings were not created.")

    # TODO:  refactor this out into its own function

    ecs_client = ECSClient(launch_type="EC2", region_name=region_name)

    ecs_client.set_cluster(cluster_name)

    # we need DPI here as a placeholder--since the DPI column is nonnullable

    container = UserContainer(
        container_id=task_id,
        user_id=None,
        cluster=cluster_name,
        ip=curr_ip,
        port_32262=curr_network_binding[32262],
        port_32263=curr_network_binding[32263],
        port_32273=curr_network_binding[32273],
        state="CREATING",
        location=region_name,
        secret_key=aeskey,
        task_definition=task_definition_arn,
        task_version=task_version,
        dpi=96,
    )
    container_sql = fractal_sql_commit(db, lambda db, x: db.session.add(x), container)
    if container_sql:
        container = UserContainer.query.get(task_id)
        fractal_logger.info(
            (
                f"Inserted container with IP address {curr_ip} and network bindings "
                f"{curr_network_binding}"
            ),
            extra={"label": str(task_id)},
        )
    else:
        fractal_logger.info("SQL insertion unsuccessful", extra={"label": str(task_id)})
        raise Exception(f"We were unable to track container {task_id} in our database.")

    cluster_usage = ecs_client.get_clusters_usage(clusters=[cluster_name])[cluster_name]
    cluster_usage["cluster"] = cluster_name
    cluster_sql = fractal_sql_commit(db, fractal_sql_update, cluster_info, cluster_usage)
    if cluster_sql:
        fractal_logger.info(
            f"Added task {str(task_id)} to cluster {cluster_name} and updated cluster info",
            extra={"label": "prewarm"},
        )

        if not current_app.testing:
            task_time_taken = time.time() - task_start_time
            logged_event_container_prewarmed(
                container.container_id, cluster_name, username="prewarm", time_taken=task_time_taken
            )

        # trigger celery task to see if manual scaling should be done
        manual_scale_cluster.delay(cluster_name, region_name)

        return user_container_schema.dump(container)
    else:
        fractal_logger.info("SQL insertion unsuccessful", extra={"label": str(task_id)})
        raise Exception(f"We were unable to assign container {task_id} to you in our database.")


@shared_task
@maintenance_track_task
def create_new_cluster(
    cluster_name: Optional[str] = None,
    instance_type: Optional[str] = "g4dn.4xlarge",
    ami: Optional[str] = "ami-0decb4a089d867dc1",
    region_name: Optional[str] = "us-east-1",
    min_size: Optional[int] = 0,
    max_size: Optional[int] = 10,
    availability_zones: Optional[List[str]] = None,
) -> Dict[str, Any]:
    """
    Create a new cluster.

    We directly call _create_new_cluster because it can easily be mocked. The __code__ attribute
    does not exist for functions with celery decorators like this one.

    Args:
        cluster_name (Optional[str]): the name of the created cluster
        instance_type (Optional[str]): size of instances to create in auto scaling group, defaults
            to t2.small
        ami (Optional[str]): AMI to use for the instances created in auto scaling group, defaults
            to an ECS-optimized, GPU-optimized Amazon Linux 2 AMI
        min_size (Optional[int]): the minimum number of containers in the auto scaling group,
            defaults to 1
        max_size (Optional[int]): the maximum number of containers in the auto scaling group,
            defaults to 10
        region_name (Optional[str]): which AWS region you're running on
        availability_zones (Optional[List[str]]): the availability zones for creating instances in
            the auto scaling group
    Returns:
        user_cluster_schema: information on cluster created
    """
    return _create_new_cluster(
        cluster_name, instance_type, ami, region_name, min_size, max_size, availability_zones
    )


def _create_new_cluster(
    cluster_name: Optional[str] = None,
    instance_type: Optional[str] = "g4dn.4xlarge",
    ami: Optional[str] = "ami-0decb4a089d867dc1",
    region_name: Optional[str] = "us-east-1",
    min_size: Optional[int] = 0,
    max_size: Optional[int] = 10,
    availability_zones: Optional[List[str]] = None,
) -> Dict[str, Any]:
    """
    See create_new_cluster. This is helpful to mock.
    """
    task_start_time = time.time()
    all_regions = RegionToAmi.query.all()

    region_to_ami = {region.region_name: region.ami_id for region in all_regions}
    if ami is None:
        ami = region_to_ami[region_name]

    fractal_logger.info(
        (
            f"Creating new cluster on ECS with instance_type {instance_type} and ami {ami} in "
            f"region {region_name}"
        ),
    )
    ecs_client = ECSClient(launch_type="EC2", region_name=region_name)
    current_task.update_state(
        state="STARTED",
        meta={
            "msg": (
                f"Creating new cluster on ECS with instance_type {instance_type} and ami {ami} in "
                f"region {region_name}"
            ),
        },
    )
    time.sleep(10)

    cluster_name, _, _, _ = ecs_client.create_auto_scaling_cluster(
        cluster_name=cluster_name,
        instance_type=instance_type,
        ami=ami,
        min_size=min_size,
        max_size=max_size,
        availability_zones=availability_zones,
    )
    fractal_logger.info(f"Created cluster {cluster_name}")

    cluster_usage = ecs_client.get_clusters_usage(clusters=[cluster_name])[cluster_name]

    cluster_usage_info = ClusterInfo(cluster=cluster_name, location=region_name, **cluster_usage)
    cluster_sql = fractal_sql_commit(db, lambda db, x: db.session.add(x), cluster_usage_info)
    if cluster_sql:
        cluster = ClusterInfo.query.get(cluster_name)
        cluster = user_cluster_schema.dump(cluster)
        fractal_logger.info(
            f"Successfully created cluster {cluster_name}", extra={"label": cluster_name}
        )

        if not current_app.testing:
            task_time_taken = time.time() - task_start_time
            logged_event_cluster_created(cluster_name, time_taken=task_time_taken)

        return cluster
    else:
        fractal_logger.info("SQL insertion unsuccessful", extra={"label": cluster_name})
        raise Exception(f"We were unable to track {cluster_name} in our database.")
