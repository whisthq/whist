from app import (
    ECSClient,
    celery_instance,
    fractalLog,
    fractalSQLCommit,
    fractalSQLUpdate,
    logging,
    db,
)
from collections import defaultdict
from .helpers.tests.aws_container import *
from app.models.hardware import UserContainer, ClusterInfo
import string
import random
import copy


def generate_name(starter_name=""):
    """
    Helper function for generating a name with a random UUID
    Args:
        starter_name (Optional[str]): starter string for the name
    Returns:
        str: the generated name
    """
    letters = string.ascii_lowercase
    return starter_name + "_" + "".join(random.choice(letters) for i in range(10))


pytest.cluster_name = generate_name("cluster")
pytest.container_name = None


@pytest.mark.container_serial
def check_test_database(input_token, admin_token):
    if os.getenv("USE_PRODUCTION_KEYS").upper() == "TRUE" or RESOURCE_GROUP != "FractalStaging":
        fractalLog(
            function="test_aws_container",
            label=None,
            logs="Not using staging database or resource group! Forcefully stopping tests.",
            level=logging.WARNING,
        )
        assert False


@pytest.mark.container_serial
def check_test_database(input_token, admin_token):
    if os.getenv("USE_PRODUCTION_KEYS").upper() == "TRUE" or RESOURCE_GROUP != "FractalStaging":
        fractalLog(
            function="test_aws_container",
            label=None,
            logs="Not using staging database or resource group! Forcefully stopping tests.",
            level=logging.WARNING,
        )
        assert False


@pytest.mark.container_serial
def test_create_cluster(
    input_token, admin_token, cluster_name=pytest.cluster_name, max_containers=10, min_containers=0
):
    cluster_name = cluster_name or pytest.cluster_name
    fractalLog(
        function="test_create_cluster",
        label="cluster/create",
        logs="Starting to create cluster {}".format(cluster_name),
    )

    resp = createCluster(
        cluster_name=cluster_name,
        instance_type="g3s.xlarge",
        ami="ami-0c82e2febb87e6d1c",  # updated for host service
        region_name="us-east-1",
        max_containers=max_containers,
        input_token=input_token,
    )

    task = queryStatus(resp, timeout=10)

    if task["status"] < 1:
        fractalLog(
            function="test_create_cluster",
            label="cluster/create",
            logs=task["output"],
            level=logging.ERROR,
        )
        assert False
    if not ClusterInfo.query.get(cluster_name):
        fractalLog(
            function="test_create_cluster",
            label="cluster/create",
            logs="Cluster was not inserted in database",
            level=logging.ERROR,
        )
        assert False
    assert True


@pytest.mark.container_serial
def test_create_container(input_token, admin_token, cluster_name=pytest.cluster_name):
    cluster_name = cluster_name or pytest.cluster_name
    fractalLog(
        function="test_create_container",
        label="container/create",
        logs="Starting to create container in cluster {}".format(cluster_name),
    )
    resp = createContainer(
        username="test-user@test.com",
        cluster_name=cluster_name,
        region_name="us-east-1",
        task_definition_arn="fractal-browsers-chrome",
        use_launch_type=False,
        input_token=input_token,
    )

    task = queryStatus(resp, timeout=50)

    if task["status"] < 1:
        fractalLog(
            function="test_create_container",
            label="container/create",
            logs=task["output"],
            level=logging.ERROR,
        )
        assert False

    if not task["result"]:
        fractalLog(
            function="test_create_container",
            label="container/create",
            logs="No container returned",
            level=logging.ERROR,
        )
        assert False
    pytest.container_name = task["result"]["container_id"]
    if not UserContainer.query.get(pytest.container_name):
        fractalLog(
            function="test_create_container",
            label="container/create",
            logs="Container was not inserted in database",
            level=logging.ERROR,
        )
        assert False

    assert True
    return task["result"]


@pytest.mark.container_serial
def test_send_commands(input_token, admin_token):
    fractalLog(
        function="test_send_commands",
        label="cluster/send_commands",
        logs="Starting to send commands to cluster {}".format(pytest.cluster_name),
    )

    resp = sendCommands(
        cluster=pytest.cluster_name,
        region_name="us-east-1",
        commands=["echo test_send_commands"],
        input_token=input_token,
    )

    task = queryStatus(resp, timeout=10)

    if task["status"] < 1:
        fractalLog(
            function="test_send_commands",
            label="cluster/send_commands",
            logs=task["output"],
            level=logging.ERROR,
        )
        assert False

    assert True


@pytest.mark.container_serial
def test_delete_container(input_token, admin_token, container_name=pytest.container_name):
    container_name = container_name or pytest.container_name
    fractalLog(
        function="test_delete_container",
        label="container/delete",
        logs="Starting to delete container {}".format(container_name),
    )

    resp = deleteContainer(
        user_id="test-user@test.com", container_name=pytest.container_name, input_token=input_token,
    )

    task = queryStatus(resp, timeout=10)

    if task["status"] < 1:
        fractalLog(
            function="test_delete_container",
            label="container/delete",
            logs=task["output"],
            level=logging.ERROR,
        )
        assert False

    if UserContainer.query.get(container_name):
        fractalLog(
            function="test_delete_container",
            label="container/delete",
            logs="Container was not deleted from database",
            level=logging.ERROR,
        )
        assert False

    assert True


@pytest.mark.container_serial
def test_delete_cluster(input_token, admin_token, cluster=pytest.cluster_name):
    cluster = cluster or pytest.cluster_name
    fractalLog(
        function="test_delete_cluster",
        label="cluster/delete",
        logs="Starting to delete cluster {}".format(cluster),
    )

    resp = deleteCluster(
        cluster=pytest.cluster_name, region_name="us-east-1", input_token=input_token,
    )

    task = queryStatus(resp, timeout=10)

    if task["status"] < 1:
        fractalLog(
            function="test_delete_cluster",
            label="cluster/delete",
            logs=task["output"],
            level=logging.ERROR,
        )
        assert False
    if ClusterInfo.query.get(cluster):
        fractalLog(
            function="test_delete_cluster",
            label="cluster/delete",
            logs="Cluster was not deleted in database",
            level=logging.ERROR,
        )
        assert False
    assert True


@pytest.mark.skipif(
    "CHOOSE_CLUSTER_TEST" not in os.environ,
    reason="This test is slow and reource heavy; run only upon explicit request",
)
def test_cluster_assignment(input_token, admin_token):
    check_test_database(input_token, admin_token)
    fractalLog(
        function="test_cluster_assignment",
        label="cluster/assign",
        logs="First deleting all clusters in database",
    )

    def delete_cluster_helper(cluster):
        fractalLog(
            function="test_cluster_assignment",
            label="cluster/delete",
            logs="Deleting cluster {} from database".format(cluster),
        )
        cluster_info = ClusterInfo.query.get(cluster)
        fractalSQLCommit(db, lambda db, x: db.session.delete(x), cluster_info)

        if ClusterInfo.query.get(cluster):
            fractalLog(
                function="test_cluster_assignment",
                label="cluster/delete",
                logs="Cluster was not deleted in database",
                level=logging.ERROR,
            )
            assert False
        assert True

    all_clusters = ClusterInfo.query.all()
    clusters = [
        cluster_info.cluster for cluster_info in all_clusters if "cluster_" in cluster_info.cluster
    ]
    if all_clusters:
        fractalLog(
            function="test_cluster_assignment",
            label="cluster/delete",
            logs="Found {num_clusters} cluster(s) already in database. Starting to delete...".format(
                num_clusters=str(len(clusters))
            ),
        )
        fractalJobRunner(delete_cluster_helper, clusters)

    else:
        fractalLog(
            function="test_cluster_assignment",
            label="cluster/delete",
            logs="No clusters found in database",
        )

    first_cluster = generate_name("cluster")
    test_create_cluster(input_token, admin_token, max_containers=2, cluster_name=first_cluster)
    clusters_to_containers = defaultdict(list)
    container_cluster = first_cluster
    container_cluster_info = ClusterInfo.query.filter_by(cluster=container_cluster).first()

    while True:
        pytest.cluster_name = None
        container_info = test_create_container(input_token, admin_token, cluster_name=None)
        new_container, new_container_cluster = (
            container_info["container_id"],
            container_info["cluster"],
        )
        clusters_to_containers[new_container_cluster].append(new_container)
        old_info = copy.copy(container_cluster_info)
        db.session.expire(container_cluster_info)
        container_cluster_info = ClusterInfo.query.get(new_container_cluster)

        if (
            old_info.registeredContainerInstancesCount < old_info.maxContainers
            or old_info.maxMemoryRemainingPerInstance > 8500
        ):
            assert container_cluster == new_container_cluster
            assert (
                container_cluster_info.maxMemoryRemainingPerInstance
                <= old_info.maxMemoryRemainingPerInstance
                or container_cluster_info.registeredContainerInstancesCount
                > old_info.registeredContainerInstancesCount
            )
        else:
            assert container_cluster != new_container_cluster
            assert (
                container_cluster_info.maxMemoryRemainingPerInstance
                >= old_info.maxMemoryRemainingPerInstance
            )
            db.session.expire(container_cluster_info)
            break

        container_cluster = new_container_cluster
    first_cluster_info = ClusterInfo.query.get(first_cluster)
    old_first_cluster_info = copy.copy(first_cluster_info)
    container_to_delete = clusters_to_containers[first_cluster].pop()
    test_delete_container(input_token, admin_token, container_to_delete)
    db.session.refresh(first_cluster_info)
    new_first_cluster_info = copy.copy(first_cluster_info)
    assert (
        new_first_cluster_info.maxMemoryRemainingPerInstance
        >= old_first_cluster_info.maxMemoryRemainingPerInstance
    )

    pytest.cluster_name = None
    container_info = test_create_container(input_token, admin_token, cluster_name=None)
    new_container, assigned_cluster = container_info["container_id"], container_info["cluster"]
    clusters_to_containers[assigned_cluster].append(new_container)
    assert assigned_cluster == first_cluster
    db.session.refresh(first_cluster_info)
    assigned_cluster_info = copy.copy(first_cluster_info)
    assert (
        assigned_cluster_info.maxMemoryRemainingPerInstance
        <= new_first_cluster_info.maxMemoryRemainingPerInstance
    )
    db.session.expire(first_cluster_info)

    for cluster in clusters_to_containers.keys():
        for container in clusters_to_containers[cluster]:
            test_delete_container(input_token, admin_token, container)
        test_delete_cluster(input_token, admin_token, cluster)
