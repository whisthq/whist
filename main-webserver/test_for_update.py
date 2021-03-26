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

    new_user = User(
        user_id=USER_ID,
        password="some-token",
        token="token",
        name="name",
        reason_for_signup="reason_for_signup",
    )
    db.session.add(new_user)

    new_cluster = ClusterInfo(
        cluster=CLUSTER,
        location="us-east-1",
        status="CREATED",
    )
    db.session.add(new_cluster)

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

    # test concurrent writes
    subprocess.run(
        f"python3 lock_testing_helper.py {CONTAINER_ID} 0 2 EDIT ONE & python3 lock_testing_helper.py {CONTAINER_ID} 1 0 EDIT TWO",
        shell=True,
    )
    container = UserContainer.query.filter_by(container_id=CONTAINER_ID).first()
    # time.sleep(1)
    assert container.state == "TWO"

    # test timeout
    result = subprocess.Popen(
        f"python3 lock_testing_helper.py {CONTAINER_ID} 0 7 EDIT ONE & python3 lock_testing_helper.py {CONTAINER_ID} 1 0 EDIT TWO",
        shell=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    output, err = result.communicate()
    if err:
        assert True
    else:
        assert False

    # test concurrent delete/write
    result = subprocess.Popen(
        f"python3 lock_testing_helper.py {CONTAINER_ID} 0 2 DELETE & python3 lock_testing_helper.py {CONTAINER_ID} 1 0 EDIT TWO",
        shell=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    output, err = result.communicate()
    if err:
        assert True
    else:
        assert False
