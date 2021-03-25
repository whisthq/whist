from app.factory import create_app
from app.models import db, User, ClusterInfo, UserContainer
from app.helpers.utils.aws.aws_resource_locks import lock_container_and_update
import os
import time
import sys
import subprocess

os.environ["REDIS_URL"] = "redis://"


app = create_app()
with app.app_context():

    CONTAINER_ID = "TEST-CONTAINER"
    CLUSTER = "test-cluster-lock"
    USER_ID = "username@test.com"

    # populate DB with necessary elements
    def initialize_db(user=False, cluster=False, container=False):
        # create user
        if user:
            new_user = User(
                user_id=USER_ID,
                password="some-token",
                token="token",
                name="name",
                reason_for_signup="reason_for_signup",
            )
            db.session.add(new_user)

        # create cluster
        if cluster:
            new_cluster = ClusterInfo(
                cluster=CLUSTER,
                location="us-east-1",
                status="CREATED",
            )
            db.session.add(new_cluster)

        # create container
        if container:
            new_container = UserContainer(
                container_id=CONTAINER_ID,
                user_id=USER_ID,
                cluster=CLUSTER,
                ip="123",
                is_assigned=True,
                port_32262=123,
                port_32263=123,
                port_32273=123,
                state="CREATING",
                location="us-east-1",
                os="Linux",
                secret_key="secret_key",
                task_definition="fractal-dev-creative-figma",
                dpi=460,
            )
            db.session.add(new_container)
        db.session.commit()

    def tear_down_resources(user=False, cluster=False, container=False):
        # delete user, cluster, and container from db
        db.session.commit()

    initialize_db(container=True)  # , True, True)

    # test concurrent writes
    subprocess.run(
        f"python3 lock_testing_helper.py {CONTAINER_ID} 0 2 EDIT ONE & python3 lock_testing_helper.py {CONTAINER_ID} 1 0 EDIT TWO",
        shell=True,
    )
    container = UserContainer.query.filter_by(container_id=CONTAINER_ID).first()
    # time.sleep(1)
    assert container.state == "TWO"

    # test concurrent delete/write
    subprocess.run(
        f"python3 lock_testing_helper.py {CONTAINER_ID} 0 2 DELETE & python3 lock_testing_helper.py {CONTAINER_ID} 1 0 EDIT TWO",
        shell=True,
    )
    container = UserContainer.query.filter_by(container_id=CONTAINER_ID).first()
    assert container is None
    initialize_db(container=True)

    # test concurrent delete/read
    subprocess.run(
        f"python3 lock_testing_helper.py {CONTAINER_ID} 0 2 DELETE &",
        shell=True,
    )
    container = UserContainer.query.filter_by(container_id=CONTAINER_ID).first()
    assert container is None

    # test concurrent write/read
    subprocess.run(
        f"python3 lock_testing_helper.py {CONTAINER_ID} 0 3 EDIT ONE &",
        shell=True,
    )
    start = time.time()
    container = UserContainer.query.filter_by(container_id=CONTAINER_ID).first()
    end = time.time()
    assert end - start > 1
    assert container.state == "ONE"
