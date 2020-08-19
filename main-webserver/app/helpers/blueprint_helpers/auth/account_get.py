from app import *
from app.helpers.utils.general.logs import *

from app.models.public import *
from app.models.hardware import *
from app.serializers.public import *
from app.serializers.hardware import *

user_schema = UserSchema()
os_disk_schema = OSDiskSchema()
secondary_disk_schema = SecondaryDiskSchema()

def codeHelper(username):
    """Fetches a user's promo code

    Parameters:
    username (str): The username

    Returns:
    json: The user's promo code
   """

    # Query database for user

    user = User.query.filter(user_id=username).first()

    # Return user's promo code

    if user:
        code = user.referral_code

        return {"code": code, "status": SUCCESS}

    else:
        return {"code": None, "status": BAD_REQUEST}


def fetchUserHelper(username):
    """Returns the user's entire info

    Args:
        username (str): Email of the user

    Returns:
        http response
    """
    user = User.query.filter(user_id=username).first()

    # Return user
    if user:
        return jsonify({"user": user_schema.dump(user), "status": SUCCESS}), SUCCESS
    else:
        return jsonify({"user": None, "status": BAD_REQUEST}), BAD_REQUEST


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

    user = User.query.get(username)
    if not user:
        return jsonify({"error": "user with email does not exist!"}), BAD_REQUEST

    os_disks = OSDisk.query.filter(user_id=username).all()
    os_disks = [os_disk_schema.dump(disk) for disk in os_disks]

    if not main:
        secondary_disks = SecondaryDisk.query.filter(user_id=username).all()
        secondary_disks = [secondary_disk_schema.dump(disk) for disk in secondary_disks]

    # Return SQL output

    fractalLog(
        function="disksHelper",
        label="{username}".format(username=username),
        logs="Disk helper function found OS disks {os_disks} and secondary disks {secondary_disks} associated with {username}".format(
            os_disks=str([disk["disk_id"] for disk in os_disks]),
            secondary_disks=str([disk["disk_id"] for disk in secondary_disks]),
            username=username,
        ),
    )
    return {"os_disks": os_disks, "secondary_disks": secondary_disks, "status": SUCCESS}



def verifiedHelper(username):
    """Checks if a user's email has been verified

    Parameters:
    username (str): The username

    Returns:
    json: Whether the email is verified
   """

    # Query database for user

    # output = fractalSQLSelect(table_name="users", params={"email": username})

    # Check if user is verified

    # if output:
    #     verified = output[0]["verified"]

    #     return {"verified": verified, "status": SUCCESS}

    # else:
    #     return {"verified": False, "status": BAD_REQUEST}

    return {"verified": True, "status": SUCCESS}
