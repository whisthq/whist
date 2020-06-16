from app import *
from app.helpers.utils.general.logs import *
from app.helpers.utils.general.sql_commands import *


def codeHelper(username):
    """Fetches a user's promo code

    Parameters:
    username (str): The username

    Returns:
    json: The user's promo code
   """

    # Query database for user

    output = fractalSQLSelect(table_name="users", params={"username": username})

    # Return user's promo code

    if output["success"] and output["rows"]:
        code = output["rows"][0]["code"]

        return {"code": code, "status": SUCCESS}

    else:
        return {"code": None, "status": BAD_REQUEST}


def disksHelper(username, main):
    """Fetches all disks associated with a username.

    Parameters:
    username (str): The username
    main (bool): True if only OS disk is desired, false if all disks are desired

    Returns:
    json: Fetched disks
   """

    # Send SQL disk select command

    fractalLog(
        function="disksHelper",
        label="{username}".format(username=username),
        logs="Disk helper function looking for disks associated with {username} and main {main}".format(
            username=username, main=str(main)
        ),
    )

    params = {"username": username, "state": "ACTIVE"}

    if main:
        params["main"] = True

    output = fractalSQLSelect("disks", params)

    # Return SQL output

    if output["success"]:
        fractalLog(
            function="disksHelper",
            label="{username}".format(username=username),
            logs="Disk helper function found disks {disks} associated with {username} and main {main}".format(
                disks=str([disk["disk_name"] for disk in output["rows"]]),
                username=username,
                main=str(main),
            ),
        )
        return {"disks": output["rows"], "status": SUCCESS}
    else:
        fractalLog(
            function="disksHelper",
            label="{username}".format(username=username),
            logs="Disk helper function failed for username {username} and main {main} with error {error}".format(
                username=username, main=str(main), error=output["error"]
            ),
        )
        return {"disks": None, "status": BAD_REQUEST}


def verifiedHelper(username):
    """Checks if a user's email has been verified

    Parameters:
    username (str): The username

    Returns:
    json: Whether the email is verified
   """

    # Query database for user

    output = fractalSQLSelect(table_name="users", params={"username": username})

    # Check if user is verified

    if output["success"] and output["rows"]:
        verified = output["rows"][0]["verified"]

        return {"verified": verified, "status": SUCCESS}

    else:
        return {"verified": False, "status": BAD_REQUEST}
