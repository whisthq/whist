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
    # populate DB with necessary container
    def initialize_db():
        # create user
        new_user = User(
            user_id="username@test.com",
            password="some-token",
            token="token",
            name="name",
            reason_for_signup="reason_for_signup",
        )
        db.session.add(new_user)

        # create cluster
        new_cluster = ClusterInfo(
            cluster="test-cluster-mia",
            location="us-east-1",
            status="CREATED",
        )
        db.session.add(new_cluster)

        # create container
        new_container = UserContainer(
            container_id=CONTAINER_ID,
            user_id=new_user.user_id,
            cluster=new_cluster.cluster,
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

    CONTAINER_ID = "HOWDY"
    # subprocess.run("python3 script1.py & python3 script2.py", shell=True)
    subprocess.run(
        f"python3 lock_testing_helper.py {CONTAINER_ID} ONE 0 10 & python3 lock_testing_helper.py {CONTAINER_ID} TWO 2 1",
        shell=True,
    )

    # test concurrent deletes

    # test concurrent write/delete

    # test concurrent write/read

    # test concurrent delete/read
