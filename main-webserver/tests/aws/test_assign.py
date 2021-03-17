"""Tests for the /container/assign endpoint."""

from http import HTTPStatus

import pytest

from app.celery.aws_ecs_creation import (
    assign_container,
    bundled_region,
    create_new_cluster,
    find_available_container,
    _get_num_extra,
    select_cluster,
)

from app.models import SupportedAppImages, User

from tests.patches import function


def test_bad_app(client, authorized):
    response = client.post(
        "/container/assign", json=dict(username=authorized.user_id, app="Bad App")
    )

    assert response.status_code == HTTPStatus.BAD_REQUEST


@pytest.mark.usefixtures("authorized")
def test_no_username(client):
    response = client.post("/container/assign", json=dict(app="VSCode"))

    assert response.status_code == HTTPStatus.UNAUTHORIZED


def test_no_app(client, authorized):
    response = client.post("/container/assign", json=dict(username=authorized.user_id))

    assert response.status_code == HTTPStatus.BAD_REQUEST


def test_no_region(client, authorized):
    response = client.post(
        "/container/assign", json=dict(username=authorized.user_id, app="VSCode")
    )

    assert response.status_code == HTTPStatus.BAD_REQUEST


def test_get_num_extra_empty(task_def_env):
    # tests the base case of _get_num_extra -- prewarm 1
    assert _get_num_extra(f"fractal-{task_def_env}-browsers-chrome", "us-east-1") == 1


def test_get_num_extra_full(bulk_container, task_def_env):
    # tests the buffer filled case of _get_num_extra -- 1 already prewarmed
    _ = bulk_container(assigned_to=None)
    assert _get_num_extra(f"fractal-{task_def_env}-browsers-chrome", "us-east-1") == 0


def test_get_num_extra_fractional(bulk_container, make_user, task_def_env):
    # tests the fractional case of _get_num_extra, ensure the ratio is right
    user = make_user()

    for _ in range(15):
        _ = bulk_container(assigned_to=user.user_id)
    preboot_num = (
        SupportedAppImages.query.filter_by(
            task_definition=f"fractal-{task_def_env}-browsers-chrome"
        )
        .first()
        .preboot_number
    )
    assert (
        _get_num_extra(f"fractal-{task_def_env}-browsers-chrome", "us-east-1") == 15.0 * preboot_num
    )


def test_get_num_extra_subtracts(bulk_container, make_user, task_def_env):
    # tests the total codepath set of _get_num_extra
    # fractional reserve and buffer filling
    user = make_user()

    for _ in range(15):
        _ = bulk_container(assigned_to=user.user_id)
    _ = bulk_container(assigned_to=None)
    preboot_num = (
        SupportedAppImages.query.filter_by(
            task_definition=f"fractal-{task_def_env}-browsers-chrome"
        )
        .first()
        .preboot_number
    )
    assert (
        _get_num_extra(f"fractal-{task_def_env}-browsers-chrome", "us-east-1")
        == (15.0 * preboot_num) - 1
    )


class NoClusterFoundException(Exception):
    """
    Just using this to detect if create_new_cluster is called in the following tests
    """

    pass


def test_no_clusters(monkeypatch):
    """
    tests that select_cluster calls create_new_cluster when called on an empty DB
    """

    def patched_create(*args, **kwargs):
        """
        Just tells us that the function was called
        """
        raise NoClusterFoundException

    monkeypatch.setattr(create_new_cluster, "delay", patched_create)
    with pytest.raises(NoClusterFoundException):
        select_cluster("us-east-1")


def test_full_cluster(monkeypatch, bulk_cluster):
    """
    tests that select_cluster calls create_new_cluster when called on an DB with
    only full clusters
    """

    def patched_create(*args, **kwargs):
        """
        Just tells us that the function was called
        """
        raise NoClusterFoundException

    bulk_cluster()
    monkeypatch.setattr(create_new_cluster, "delay", patched_create)
    with pytest.raises(NoClusterFoundException):
        select_cluster("us-east-1")


def test_test_cluster(monkeypatch, bulk_cluster):
    """
    tests that select_cluster calls create_new_cluster when called on an DB with
    only test clusters
    """

    def patched_create(*args, **kwargs):
        """
        Just tells us that the function was called
        """
        raise NoClusterFoundException

    bulk_cluster(cluster_name="test-cluster", maxContainers=10, registeredContainerInstancesCount=0)
    monkeypatch.setattr(create_new_cluster, "delay", patched_create)
    with pytest.raises(NoClusterFoundException):
        select_cluster("us-east-1")


def test_too_full_cluster(monkeypatch, bulk_cluster):
    """
    tests that select_cluster calls create_new_cluster when called on an DB with
    only clusters that're too full
    """

    def patched_create(*args, **kwargs):
        """
        Just tells us that the function was called
        """
        raise NoClusterFoundException

    bulk_cluster(
        maxContainers=10,
        registeredContainerInstancesCount=10,
        maxMemoryRemainingPerInstance=10,
    )
    monkeypatch.setattr(create_new_cluster, "delay", patched_create)
    with pytest.raises(NoClusterFoundException):
        select_cluster("us-east-1")


def test_single_normal_cluster(monkeypatch, bulk_cluster):
    """
    tests that select_cluster returns a fully normal/running cluster
    """

    def patched_create(*args, **kwargs):
        """
        Just tells us that the function was called
        """
        return

    cluster_name = "base_cluster"
    bulk_cluster(
        cluster_name=cluster_name,
        maxContainers=10,
        registeredContainerInstancesCount=1,
        maxMemoryRemainingPerInstance=10,
    )
    monkeypatch.setattr(create_new_cluster, "delay", patched_create)
    assert select_cluster("us-east-1") == cluster_name


def test_sorted_normal_clusters(monkeypatch, bulk_cluster):
    """
    tests that select_cluster returns the available cluster with the most tasks
    """

    def patched_create(*args, **kwargs):
        """
        Just tells us that the function was called
        """
        return

    cluster_name = "base_cluster_{}"
    for i in range(10):
        bulk_cluster(
            cluster_name=cluster_name.format(i),
            maxContainers=10,
            runningTasksCount=i + 1,
            registeredContainerInstancesCount=1,
            maxMemoryRemainingPerInstance=10,
        )
    monkeypatch.setattr(create_new_cluster, "delay", patched_create)
    assert select_cluster("us-east-1") == cluster_name.format(9)


def test_single_nearfull_cluster(monkeypatch, bulk_cluster):
    """
    tests that select_cluster returns a fully normal/running cluster
    that's almost full
    """

    def patched_create(*args, **kwargs):
        """
        Just tells us that the function was called
        """
        return

    cluster_name = "base_cluster"
    bulk_cluster(
        cluster_name=cluster_name,
        maxContainers=10,
        registeredContainerInstancesCount=10,
        maxMemoryRemainingPerInstance=10000,
    )
    monkeypatch.setattr(create_new_cluster, "delay", patched_create)
    assert select_cluster("us-east-1") == cluster_name


def test_newly_created_empty_cluster(monkeypatch, bulk_cluster):
    """
    tests that select_cluster returns a newly created cluster
    """

    def patched_create(*args, **kwargs):
        """
        Just tells us that the function was called
        """
        return

    cluster_name = "base_cluster"
    bulk_cluster(
        cluster_name=cluster_name,
        maxContainers=10,
        registeredContainerInstancesCount=0,
        maxMemoryRemainingPerInstance=0,
    )
    monkeypatch.setattr(create_new_cluster, "delay", patched_create)
    assert select_cluster("us-east-1") == cluster_name


def test_newly_created_present_cluster(monkeypatch, bulk_cluster):
    """
    tests that select_cluster returns a newly created cluster with a live instance
    """

    def patched_create(*args, **kwargs):
        """
        Just tells us that the function was called
        """
        return

    cluster_name = "base_cluster"
    bulk_cluster(
        cluster_name=cluster_name,
        maxContainers=10,
        registeredContainerInstancesCount=1,
        maxMemoryRemainingPerInstance=0,
    )
    monkeypatch.setattr(create_new_cluster, "delay", patched_create)
    assert select_cluster("us-east-1") == cluster_name


def test_basic_find_unassigned(task_def_env):
    # ensures that find-available returns None on an empty DB
    assert find_available_container("us-east-1", f"fractal-{task_def_env}-browsers-chrome") is None


def test_find_unassigned(bulk_container, task_def_env):
    # ensures that find-available returns an available unassigned container
    bulk_container(assigned_to=None, location="us-east-1")
    assert (
        find_available_container("us-east-1", f"fractal-{task_def_env}-browsers-chrome") is not None
    )


def test_dont_find_unassigned(bulk_container, task_def_env, make_user):
    # ensures that find-available doesn't return an assigned container
    bulk_container(assigned_to=make_user().user_id, location="us-east-1")
    assert find_available_container("us-east-1", f"fractal-{task_def_env}-browsers-chrome") is None


def test_find_unassigned_in_unbundled(bulk_container, task_def_env):
    # ensures that find-available returns an available unassigned container in a nonbundled region
    bulk_container(assigned_to=None, location="ca-central-1")
    assert (
        find_available_container("ca-central-1", f"fractal-{task_def_env}-browsers-chrome")
        is not None
    )


def test_dont_find_unassigned_in_unbundled(bulk_container, task_def_env, make_user):
    # ensures that find-available doesn't return an assigned container in a nonbundled region
    bulk_container(assigned_to=make_user().user_id, location="ca-central-1")
    assert (
        find_available_container("ca-central-1", f"fractal-{task_def_env}-browsers-chrome") is None
    )


@pytest.fixture
def test_assignment_replacement_code(bulk_container, task_def_env, make_user):
    """
    generates a function which, given a region, returns a multistage test
    of the unassigned container finding code
    """

    def _test_find_unassigned(location):
        """
        tests 4 properties of our find unassigned algorithm:
        1) we don't return an assigned container in the requested region
        2) we don't return an assigned container in a replacement region
        3) we return an unassigned container in a replacement region
        4) if available, we use the requested region rather than a replacement one
        these properties are tested in order
        """
        replacement_region = bundled_region[location][0]
        bulk_container(assigned_to=make_user().user_id, location=location)
        assert (
            find_available_container(location, f"fractal-{task_def_env}-browsers-chrome") is None
        ), f"we assigned an already assigned container in the main region, {location}"
        bulk_container(assigned_to=make_user().user_id, location=replacement_region)
        assert (
            find_available_container(location, f"fractal-{task_def_env}-browsers-chrome") is None
        ), f"we assigned an already assigned container in the secondary region, {replacement_region}"
        bulk_container(
            assigned_to=None, location=replacement_region, container_id="replacement-container"
        )
        assert (
            find_available_container(location, f"fractal-{task_def_env}-browsers-chrome")
            is not None
        ), f"we failed to find the unassigned container in the replacement region {replacement_region}"
        bulk_container(assigned_to=None, location=location, container_id="main-container")
        container = find_available_container(location, f"fractal-{task_def_env}-browsers-chrome")
        assert (
            container is not None and container.container_id == "main-container"
        ), f"we failed to find the unassigned container in the main region {location}"

    return _test_find_unassigned


@pytest.mark.parametrize(
    "location",
    bundled_region.keys(),
)
def test_assignment_logic(test_assignment_replacement_code, location):
    # ensures that replacement will work for all regions
    test_assignment_replacement_code(location)


@pytest.mark.parametrize(
    "email_domain, subscribed, status_code",
    (
        ("fractal.co", True, HTTPStatus.ACCEPTED),
        ("fractal.co", False, HTTPStatus.ACCEPTED),
        ("gmail.com", True, HTTPStatus.ACCEPTED),
        ("gmail.com", False, HTTPStatus.PAYMENT_REQUIRED),
    ),
)
@pytest.mark.usefixtures("celery_app")
@pytest.mark.usefixtures("celery_worker")
def test_payment(client, email_domain, make_authorized_user, monkeypatch, status_code, subscribed):
    user = make_authorized_user(domain=email_domain)

    monkeypatch.delattr(User, "subscribed")
    monkeypatch.setattr(assign_container, "run", function())
    monkeypatch.setattr(user, "subscribed", subscribed, raising=False)

    response = client.post(
        "/container/assign",
        json={
            "username": user.user_id,
            "app": "Google Chrome",
            "region": "us-east-1",
        },
    )

    assert response.status_code == status_code
