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

    # actually modify state
    start = time.time()

    print("State", NEW_STATE, "waiting to lock for", WAIT_BEFORE_LOCK, "seconds")
    time.sleep(WAIT_BEFORE_LOCK)

    container = UserContainer.query.with_for_update().filter_by(container_id=CONTAINER_ID).first()

    if container is None:
        initialize_db()
        container = (
            UserContainer.query.with_for_update().filter_by(container_id=CONTAINER_ID).first()
        )

    print("State", NEW_STATE, "waiting to commit for", WAIT_BEFORE_COMMIT, "seconds")
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
