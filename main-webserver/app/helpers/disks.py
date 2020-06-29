from app.utils import *
from app import *
from app.logger import *
from .general import *
from .vms import *


def createDiskEntry(
    disk_name, vm_name, username, location, disk_size=120, main=True, state="ACTIVE"
):
    """Adds a disk to the disks SQL database

    Parameters:
    disk_name(str): The name of the disk to add
    vmName (str): The name of the VM that it is attached to
    username (str): The username of the disk's respective user
    location (str): The Azure region of the vm
    state (str): The state of the disk (default is "ACTIVE")
   """

    with engine.connect() as conn:
        commandDisk = text(
            """
            INSERT INTO disks("disk_name", "vm_name", "username", "location", "state", "disk_size", "main")
            VALUES(:diskname, :vmname, :username, :location, :state, :disk_size, :main)
            """
        )
        paramsDisk = {
            "diskname": disk_name,
            "vmname": vm_name,
            "username": username,
            "location": location,
            "state": state,
            "disk_size": disk_size,
            "main": main,
        }

        conn.execute(commandDisk, **paramsDisk)

        conn.close()

        assignSettingsToDisk(disk_name, "Fractal")

        # TODO: this method should take the OS


def genDiskName():
    """Generates a unique name for a disk

    Returns:
        str: The generated name
    """
    with engine.connect() as conn:
        oldDisks = [
            cell[0] for cell in list(conn.execute('SELECT "disk_name" FROM disks'))
        ]
        diskName = genHaiku(1)[0]
        while diskName in oldDisks:
            diskName = genHaiku(1)[0]

        diskName = str(diskName) + "_disk"

        return diskName


def genPassword():
    """Generates a random password for a disk, from an alphabet of lowercase letters, numbers, and acceptable punctuation

    Returns:
        str: The generated password
    """

    # alphabet = string.ascii_lowercase + string.digits + ',./;[]'
    # password = ''.join(secrets.choice(alphabet) for i in range(24))
    password = '6ifq59b;c],c6t.kh.5iw,m/vp.3a;;i'

    return password

def getVMSize(disk_name):
    """Gets the size of the vm

    Args:
        disk_name (str): Name of the disk attached to the vm

    Returns:
        str: The size of the disk attached to the vm
    """
    command = text(
        """
        SELECT * FROM disks WHERE "disk_name" = :disk_name
        """
    )
    params = {"disk_name": disk_name}
    with engine.connect() as conn:
        disks_info = cleanFetchedSQL(conn.execute(command, **params).fetchone())
        conn.close()
        return disks_info["vm_size"]


def getDiskSettings(disk_name):
    """Gets the disk settings for the disk

    Args:
        disk_name (str): Name of the disk

    Returns:
        dict: disk settings row
    """
    command = text(
        """
        SELECT * FROM disk_settings WHERE "disk_name" = :disk_name
        """
    )
    params = {"disk_name": disk_name}
    with engine.connect() as conn:
        disks_info = cleanFetchedSQL(conn.execute(command, **params).fetchone())
        conn.close()
        return disks_info


def fetchUserDisks(username, show_all=False, main=True, ID=-1):
    """Fetches all disks associated with the user

    Args:
        username (str): The username. If username is null, it fetches all disks
        show_all (bool, optional): Whether or not to select all disks regardless of state, vs only disks with ACTIVE state. Defaults to False.
        ID (int, optional): Papertrail logging ID. Defaults to -1.

    Returns:
        array: An array of the disks
    """
    if username:
        if not show_all:
            sendInfo(
                ID,
                "Fetching all disks associated with {} state ACTIVE".format(username),
            )

            command = text(
                """
                SELECT disks.*, disk_settings.* FROM disks LEFT JOIN disk_settings ON disks.disk_name = disk_settings.disk_name WHERE "username" = :username AND "state" = :state
                """
            )
            params = {"username": username, "state": "ACTIVE"}
            with engine.connect() as conn:
                sendInfo(ID, "Connection with Postgres established")

                disks_info = cleanFetchedSQL(conn.execute(command, **params).fetchall())
                conn.close()

                if not disks_info:
                    return []

                if main:
                    disks_info = [disk for disk in disks_info if disk["main"]]

                return disks_info
        else:
            sendInfo(
                ID,
                "Fetching all disks associated with {} regardless of state".format(
                    username
                ),
            )

            command = text(
                """
                SELECT * FROM disks WHERE "username" = :username
                """
            )
            params = {"username": username}
            with engine.connect() as conn:
                sendInfo(ID, "Connection with Postgres established")

                disks_info = cleanFetchedSQL(conn.execute(command, **params).fetchall())
                conn.close()

                if disks_info:
                    sendInfo(ID, "Disk names fetched and Postgres connection closed")
                else:
                    sendWarning(ID, "No disk found for {}. Postgres connection closed")
                    return []

                if main:
                    disks_info = [disk for disk in disks_info if disk["main"]]

                return disks_info
    else:
        sendInfo(ID, "Fetching all disks in Postgres")

        command = text(
            """
            SELECT * FROM disks
            """
        )
        params = {}
        with engine.connect() as conn:
            sendInfo(ID, "Connection with Postgres established")

            disks_info = cleanFetchedSQL(conn.execute(command, **params).fetchall())
            conn.close()

            if disks_info:
                sendInfo(ID, "Disk names fetched and Postgres connection closed")
            else:
                sendWarning(ID, "No disk found in Postgres. Postgres connection closed")

            if main:
                disks_info = [disk for disk in disks_info if disk["main"]]

            return disks_info


def fetchSecondaryDisks(username, ID=-1):
    """Fetches all non-OS disks associated with the user

    Args:
        username (str): The username. If username is null, it fetches all disks
        show_all (bool, optional): Whether or not to select all disks regardless of state, vs only disks with ACTIVE state. Defaults to False.
        ID (int, optional): Papertrail logging ID. Defaults to -1.

    Returns:
        array: An array of the disks
    """
    sendInfo(
        ID,
        "Fetching all non-OS disks associated with {} state ACTIVE".format(username),
    )

    command = text(
        """
        SELECT * FROM disks WHERE "username" = :username AND "state" = :state AND "main" = :main
        """
    )
    params = {"username": username, "state": "ACTIVE", "main": False}
    with engine.connect() as conn:
        sendInfo(ID, "Connection with Postgres established")

        disks_info = cleanFetchedSQL(conn.execute(command, **params).fetchall())
        conn.close()

        if disks_info:
            sendInfo(ID, "Disk names fetched and Postgres connection closed")
        else:
            sendWarning(ID, "No non-OS disks found for {}. Postgres connection closed")

        return disks_info


def updateDisk(disk_name, vm_name, location):
    """Updates the vm name and location properties of the disk. If no disk with the provided name exists, create a new disk entry

    Args:
        disk_name (str): Name of disk to update
        vm_name (str): Name of the new vm that the disk is attached to
        location (str): The new Azure region of the disk
    """
    command = text(
        """
        SELECT * FROM disks WHERE "disk_name" = :disk_name
        """
    )
    params = {"disk_name": disk_name}
    with engine.connect() as conn:
        disk = cleanFetchedSQL(conn.execute(command, **params).fetchone())
        if disk:
            if location:
                command = text(
                    """
                    UPDATE disks
                    SET "vm_name" = :vm_name, "location" = :location
                    WHERE
                    "disk_name" = :disk_name
                """
                )
                params = {
                    "vm_name": vm_name,
                    "location": location,
                    "disk_name": disk_name,
                }
            else:
                command = text(
                    """
                    UPDATE disks
                    SET "vm_name" = :vm_name
                    WHERE
                    "disk_name" = :disk_name
                """
                )
                params = {"vm_name": vm_name, "disk_name": disk_name}
        else:
            if location:
                command = text(
                    """
                    INSERT INTO disks("disk_name", "vm_name", "location")
                    VALUES(:disk_name, :vm_name, :location)
                    """
                )
                params = {
                    "vm_name": vm_name,
                    "location": location,
                    "disk_name": disk_name,
                }
            else:
                command = text(
                    """
                    INSERT INTO disks("disk_name", "vm_name")
                    VALUES(:disk_name, :vm_name)
                    """
                )
                params = {"vm_name": vm_name, "disk_name": disk_name}

        conn.execute(command, **params)
        conn.close()


def assignUserToDisk(disk_name, username):
    """Assigns a user to a disk

    Args:
        disk_name (str): Disk that the user will be assigned to
        username (str): Username of the user
    """
    command = text(
        """
        UPDATE disks SET "username" = :username WHERE "disk_name" = :disk_name
        """
    )
    params = {"username": username, "disk_name": disk_name}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()


def associateVMWithDisk(vm_name, disk_name):
    """Associates a VM with a disk on the v_ms sql table

    Args:
        vm_name (str): The name of the vm
        disk_name (str): The name of the disk
    """
    username = mapDiskToUser(disk_name)
    username = username if username else ""
    command = text(
        """
        UPDATE v_ms
        SET "disk_name" = :disk_name, "username" = :username
        WHERE
        "vm_name" = :vm_name
        """
    )
    params = {"vm_name": vm_name, "disk_name": disk_name, "username": username}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()

def assignSettingsToDisk(disk_name, admin_username):
    """Assigns settings for a disk

    Args:
        disk_name (str): The name of the disk
    """

    rand_password = genPassword()
    command = text(
        """
        INSERT INTO disk_settings ("admin_username", "admin_password", "disk_name", "branch", "using_stun") VALUES (:admin_username, :admin_password, :disk_name, :branch, :using_stun)
        """
    )

    params = {"admin_username": admin_username, "admin_password": rand_password, "disk_name": disk_name, "branch": "master", "using_stun": False}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()

def fetchAllDisks():
    """Fetches all the disks from disks table, LEFT joined with disk_settings

    Returns:
        arr[dict]: An array of all the disks in the disks sql table
    """
    command = text(
        """
        SELECT disks.*, disk_settings.using_stun
        FROM disks
        LEFT JOIN disk_settings ON disks.disk_name = disk_settings.disk_name
        """
    )

    params = {}

    with engine.connect() as conn:
        disks = cleanFetchedSQL(conn.execute(command, **params).fetchall())
        return disks


def deleteDiskFromTable(disk_name):
    """Deletes a disk from the disks sql table

    Args:
        disk_name (str): The name of the disk to delete
    """
    command = text(
        """
        DELETE FROM disks WHERE "disk_name" = :disk_name
        """
    )
    params = {"disk_name": disk_name}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()


def deleteDiskSettingsFromTable(disk_name):
    """Deletes a disk from the disk_settings sql table

    Args:
        disk_name (str): The name of the disk to delete settings for
    """
    command = text(
        """
        DELETE FROM disk_settings WHERE "disk_name" = :disk_name
        """
    )
    params = {"disk_name": disk_name}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()


def scheduleDiskDelete(disk_name, date, ID=-1):
    """Schedule a disk to be deleted later

    Args:
        disk_name (str): Name of the disk to be deleted
        date (str): The date at which it should be deleted. In the format mm/dd/yyyy, hh:mm (24 hour time)
        ID (int, optional): The unique paprtrail loggin id. Defaults to -1.
    """
    dateString = dateToString(date)
    command = text(
        """
        UPDATE disks
        SET "delete_date" = :delete_date, "state" = :'TO_BE_DELETED'
        WHERE
        "disk_name" = :disk_name
        """
    )
    params = {"disk_name": disk_name, "delete_date": dateString}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()
    sendInfo(ID, "Fetching all disks associated with state ACTIVE")


def mapDiskToUser(disk_name, ID=-1):
    """Find the user that is associated with the disk

    Args:
        disk_name (str): The name of the disk of interest
        ID (int, optional): Papertrail loggin id. Defaults to -1.

    Returns:
        str: The username associated with the disk
    """
    command = text(
        """
        SELECT * FROM disks WHERE "disk_name" = :disk_name
        """
    )

    params = {"disk_name": disk_name}

    with engine.connect() as conn:
        disk = cleanFetchedSQL(conn.execute(command, **params).fetchone())
        if disk:
            sendInfo(
                ID, "Disk {} belongs to user {}".format(disk_name, disk["username"])
            )
            return disk["username"]
        sendWarning(ID, "No username found for disk {}".format(disk_name))
        return None


def updateDiskState(disk_name, state):
    """Updates the state of a disk in the disks sql table

    Args:
        disk_name (str): Name of the disk to update
        state (str): The new state of the disk
    """
    command = text(
        """
        UPDATE disks
        SET state = :state
        WHERE
        "disk_name" = :disk_name
        """
    )
    params = {"state": state, "disk_name": disk_name}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()


def assignVMSizeToDisk(disk_name, vm_size):
    """Updates the vm_size field for a disk

    Args:
        disk_name (str): The name of the disk to update
        vm_size (str): The new size of the vm the disk is attached to
    """
    command = text(
        """
        UPDATE disks SET "vm_size" = :vm_size WHERE "disk_name" = :disk_name
        """
    )
    params = {"vm_size": vm_size, "disk_name": disk_name}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()


def createDiskFromImageHelper(username, location, vm_size, operating_system, ID=-1):
    """Creates a disk from an image, and assigns users and vm_sizes to the disk

    Args:
        username (str): The username of the user that will be assigned to the disk
        location (str): The Azure region of the new disk
        vm_size (str): The size of the vm associated with the new disk
        ID (int, optional): Unique papertrail logging id. Defaults to -1.

    Returns:
        int: 200 for success, 400 for error
    """
    disk_name = genDiskName()
    sendInfo(ID, "Preparing to create disk {} from an image".format(disk_name))
    _, compute_client, _ = createClients()

    try:
        admin_username = "Fractal"

        ORIGINAL_DISK = "Fractal_Disk_Eastus"
        if location == "southcentralus":
            ORIGINAL_DISK = "Fractal_Disk_Southcentralus"
        elif location == "northcentralus":
            ORIGINAL_DISK = "Fractal_Disk_Northcentralus"

        if operating_system == "Linux":
            admin_username = "fractal"
            if not location == "northcentralus":
                return {
                    "status": 402,
                    "disk_name": None,
                    "error": "{} disk not available in {}".format(
                        operating_system, location
                    ),
                }
            ORIGINAL_DISK = ORIGINAL_DISK + "_Linux"

        disk_image = compute_client.disks.get("Fractal", ORIGINAL_DISK)
        sendInfo(
            ID,
            "Image found. Preparing to create {} disk {} with location {} under {} attached to a {} VM".format(
                operating_system, disk_name, location, username, vm_size
            ),
        )
        async_disk_creation = compute_client.disks.create_or_update(
            os.environ["VM_GROUP"],
            disk_name,
            {
                "location": location,
                "creation_data": {
                    "create_option": DiskCreateOption.copy,
                    "source_resource_id": disk_image.id,
                    "managed_disk": {"storage_account_type": "StandardSSD_LRS"},
                },
            },
        )
        sendDebug(ID, "Disk clone command sent. Waiting on disk to create")
        new_disk = async_disk_creation.result()
        sendInfo(ID, "Disk {} successfully created from image".format(disk_name))

        updateDisk(disk_name, "", location)
        assignUserToDisk(disk_name, username)
        assignVMSizeToDisk(disk_name, vm_size)
        assignSettingsToDisk(disk_name, admin_username)

        return {"status": 200, "disk_name": disk_name}
    except Exception as e:
        sendCritical(ID, str(e))

        sendInfo(ID, "Attempting to delete the disk {}".format(disk_name))
        os_disk_delete = compute_client.disks.delete(os.getenv("VM_GROUP"), disk_name)
        os_disk_delete.wait()
        sendInfo(
            ID,
            "Disk {} deleted due to a critical error in disk creation".format(
                disk_name
            ),
        )

        time.sleep(30)
        return {"status": 400, "disk_name": None}


def setDiskVersion(disk_name, branch):
    """Sets the version of the protocol running on the disk. Master is latest stable version, staging is second latest version

    Args:
        disk_name (str): The name of the disk
        branch (str): "master", "staging"
    """
    command = text(
        """
        UPDATE disks
        SET branch = :branch
        WHERE
        "disk_name" = :disk_name
        """
    )
    params = {"branch": branch, "disk_name": disk_name}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()


def setDiskVersion(disk_name, branch, ID=-1):
    """Sets the version of the protocol running on the disk. Master is latest stable version, staging is second latest version

    Args:
        disk_name (str): The name of the disk
        branch (str): "master", "staging"
    """

    command = text(
        """
        UPDATE disks
        SET branch = :branch
        WHERE
        "disk_name" = :disk_name
        """
    )
    params = {"branch": branch, "disk_name": disk_name}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()


def setUpdateAccepted(disk_name, accepted, ID=-1):
    """Sets the version of the protocol running on the disk. Master is latest stable version, staging is second latest version

    Args:
        disk_name (str): The name of the disk
        accepted (str): Boolean if they have been accepted
    """

    sendInfo(ID, "Disk {} has set acceptedUpdate to {}".format(disk_name, accepted))

    command = text(
        """
        UPDATE disks
        SET has_accepted_update = :accepted
        WHERE
        "disk_name" = :disk_name
        """
    )
    params = {"accepted": accepted, "disk_name": disk_name}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()


def fetchDiskApps(disk_name, ID=-1):
    """Fetches user's desired apps for a disk

    Args:
        disk_name (str): The name of the disk
        ID (int, optional): The papertrail logging ID. Defaults to -1.

    Returns:
        list: representing the app names of the user's desired apps in the disk_apps sql database
    """
    sendInfo(ID, "Fetching apps for disk {}".format(disk_name))

    if disk_name:
        command = text(
            """
            SELECT "app_name" FROM disk_apps WHERE "disk_name" = :disk_name
            """
        )
        params = {"disk_name": disk_name}
        with engine.connect() as conn:
            apps = cleanFetchedSQL(conn.execute(command, **params).fetchall())
            conn.close()
            return apps


def insertDiskApps(disk_name, apps, ID=-1):
    """Fetches user's desired apps

    Args:
        disk_name (str): The disk_name
        apps (list): The list of apps
        ID (int, optional): The papertrail logging ID. Defaults to -1.
    """

    with engine.connect() as conn:
        try:
            if disk_name:
                for app_name in apps:
                    sendInfo(
                        ID, "Inserting app {} for disk {}".format(app_name, disk_name)
                    )

                    command = text(
                        """
                        INSERT INTO disk_apps (disk_name, app_name) VALUES (:disk_name, :app_name)
                        """
                    )
                    params = {"disk_name": disk_name, "app_name": app_name}

                    conn.execute(command, **params)

        except:
            return 400

        conn.close()
        return 200


def insertDiskSetting(disk_name, branch, using_stun):
    with engine.connect() as conn:
        command = text(
            """
            SELECT * FROM disk_settings WHERE "disk_name" = :disk_name
            """
        )

        params = {"disk_name": disk_name}

        disks = cleanFetchedSQL(conn.execute(command, **params).fetchall())

        if disks:
            command = text(
                """
                UPDATE disk_settings
                SET branch = :branch, using_stun = :using_stun
                WHERE
                "disk_name" = :disk_name
                """
            )
            params = {
                "disk_name": disk_name,
                "branch": branch,
                "using_stun": using_stun,
            }
            conn.execute(command, **params)
            conn.close()
        else:
            command = text(
                """
                INSERT INTO disk_settings("disk_name", "branch", "using_stun")
                VALUES(:disk_name, :branch, :using_stun)
                """
            )
            params = {
                "disk_name": disk_name,
                "branch": branch,
                "using_stun": using_stun,
            }

            conn.execute(command, **params)
            conn.close()


def modifyDiskSetting(disk_name, settings_dict):
    with engine.connect() as conn:
        for setting_name, setting in settings_dict.items():
            command = text(
                """
                UPDATE disk_settings
                SET {setting_name} = :{setting_name}
                WHERE
                "disk_name" = :disk_name
                """.format(
                    setting_name=setting_name
                )
            )

            params = {setting_name: setting, "disk_name": disk_name}

            conn.execute(command, **params)

        conn.close()


def fetchDiskSetting(disk_name, setting_name):
    with engine.connect() as conn:
        command = text(
            """
            SELECT * FROM disk_settings WHERE "disk_name" = :disk_name
            """
        )

        params = {"disk_name": disk_name}

        disk_info = cleanFetchedSQL(conn.execute(command, **params).fetchone())

        if disk_info:
            return disk_info[setting_name]
        else:
            return None


def fetchDiskInfo(disk_name):
    """Fetches all disks associated with the user

    Args:
        username (str): The username. If username is null, it fetches all disks
        show_all (bool, optional): Whether or not to select all disks regardless of state, vs only disks with ACTIVE state. Defaults to False.
        ID (int, optional): Papertrail logging ID. Defaults to -1.

    Returns:
        array: An array of the disks
    """
    command = text(
        """
        SELECT * FROM disks WHERE "disk_name" = :disk_name
        """
    )
    params = {"disk_name": disk_name}
    with engine.connect() as conn:
        disk_info = cleanFetchedSQL(conn.execute(command, **params).fetchone())
        conn.close()

        return disk_info
