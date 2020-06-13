from app import *
from app.helpers.utils.sql_commands import *
from app.helpers.utils.logs import *


def disksHelper(username, main):
    logger.info(
        "Disk helper function fetching all disks with username {username} and main {main}".format(
            username=username, main=str(main)
        )
    )

    print("test")

    params = {"username": username, "state": "ACTIVE"}

    if main:
        params["main"] = True

    output = fractalSQLSelect("disks", params)

    logger.info(
        "Disk helper function found {output} for username {username} and main {main}".format(
            output=output, username=username, main=str(main)
        )
    )

    if output["success"]:
        return {"disks": output["rows"], "status": 200}
    else:
        return {"disks": None, "status": 400}
