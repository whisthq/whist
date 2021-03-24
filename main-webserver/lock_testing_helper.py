from app.factory import create_app
from app.models import db, User, ClusterInfo, UserContainer
from app.helpers.utils.aws.aws_resource_locks import lock_container_and_update
import os
import time
import sys

os.environ["REDIS_URL"] = "redis://"

CONTAINER_ID = sys.argv[1]
NEW_STATE = sys.argv[2]
WAIT_BEFORE_LOCK = int(sys.argv[3])
WAIT_BEFORE_COMMIT = int(sys.argv[4])

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

    # actually modify state
    start = time.time()

    print("State", NEW_STATE, "waiting for", WAIT_BEFORE_LOCK, "seconds")
    time.sleep(WAIT_BEFORE_LOCK)

    container = UserContainer.query.with_for_update().filter_by(container_id=CONTAINER_ID).first()

    if container is None:
        initialize_db()
        container = (
            UserContainer.query.with_for_update().filter_by(container_id=CONTAINER_ID).first()
        )

    print("State", NEW_STATE, "waiting for", WAIT_BEFORE_COMMIT, "seconds")
    time.sleep(WAIT_BEFORE_COMMIT)

    # update state and commit
    container.state = NEW_STATE
    db.session.commit()

    end = time.time()
    print(end - start)


# _______________________ TRASH ________________________
# with db.session.get_bind().connect() as con:
#     rs = con.execute("select * from pg_locks;")
#     for row in rs:
#         print(row)

# base_container = UserContainer.query.filter_by(
#     container_id="id1"
# ).first()  # lock using with_for_update()
# print(base_container.state)
