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
    # test concurrent writes

    CONTAINER_ID = "HOWDY"

    subprocess.Popen(
        [
            "python3",
            "lock_testing_helper.py",
            CONTAINER_ID,
            "ONE",
            str(0),
            str(10),
            "&",
            "python3",
            "lock_testing_helper.py",
            CONTAINER_ID,
            "TWO",
            str(2),
            str(1),
        ],
        stdout=subprocess.PIPE,
    ).communicate()

    # test concurrent deletes

    # test concurrent write/delete

    # test concurrent write/read

    # test concurrent delete/read
