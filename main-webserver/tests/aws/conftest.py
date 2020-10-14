"""Test fixtures for code that needs access to AWS."""

import os
import uuid

from contextlib import contextmanager
from random import getrandbits as randbits

import pytest

from app import db
from app.models.hardware import ClusterInfo, UserContainer
from app.models.public import User


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
def container(cluster, region, user):
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
            location=region,
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
def region():
    """Provide a standard region in which to create all test resources."""

    return "us-east-1"


@pytest.fixture
def user():
    """Add a row to the users table for testing.

    Returns:
        An instance of the User model.
    """

    u = User(user_id=f"test-user-{uuid.uuid4()}", password="")

    db.session.add(u)
    db.session.commit()

    yield u

    db.session.delete(u)
    db.session.commit()
