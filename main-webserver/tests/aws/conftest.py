"""Test fixtures for code that needs access to AWS."""

import os
import uuid

from contextlib import contextmanager
from random import getrandbits as randbits

import boto3
import pytest

from app.models import ClusterInfo, db, UserContainer

from .config import (
    EC2_ROLE_FOR_ECS_POLICY_ARN,
    ECS_INSTANCE_ROLE_POLICY_DOCUMENT,
)
from .services import iam


@pytest.fixture
def aws():
    """Create a new boto client session."""

    yield boto3.session.Session()


@pytest.fixture
def cluster():
    """Add a row to the cluster_info of the database for testing.

    Returns:
        An instance of the ClusterInfo model.
    """

    c = ClusterInfo(cluster=f"test-cluster-{uuid.uuid4()}")

    db.session.add(c)
    db.session.commit()

    yield c

    db.session.delete(c)
    db.session.commit()


@pytest.fixture
def container(cluster, user):
    """Add a row to the user_containers table for testing.

    Returns:
        A context manager that populates the user_containers table with a test
        row whose state column is set to initial_state.
    """

    @contextmanager
    def _container(initial_state="CREATING"):
        """Create a dummy container for testing.

        Arguments:
            initial_state: The initial value with which the new row's state
                column should be populated.

        Yields:
            An instance of the UserContainer model.
        """

        c = UserContainer(
            container_id=f"test-container-{uuid.uuid4()}",
            ip=f"{randbits(7)}.{randbits(7)}.{randbits(7)}.{randbits(7)}",
            location="us-east-1",
            os="Linux",
            state=initial_state,
            user_id=user.user_id,
            port_32262=32262,
            port_32263=32263,
            port_32273=32273,
            cluster=cluster.cluster,
            secret_key=os.urandom(8).hex(),
        )

        db.session.add(c)
        db.session.commit()

        yield c

        db.session.delete(c)
        db.session.commit()

    return _container


@pytest.fixture
def ecs_instance_role(aws, iam):
    """Create a version of the ecsInstanceRole for testing purposes."""

    role_name = f"test-role-{uuid.uuid4()}"
    iam_client = aws.client("iam")

    iam_client.create_role(
        RoleName=role_name,
        AssumeRolePolicyDocument=ECS_INSTANCE_ROLE_POLICY_DOCUMENT,
    )

    iam = aws.resource("iam")
    role = iam.Role(role_name)

    role.attach_policy(PolicyArn=EC2_ROLE_FOR_ECS_POLICY_ARN)

    yield role

    role.detach_policy(PolicyArn=EC2_ROLE_FOR_ECS_POLICY_ARN)
    role.delete()
