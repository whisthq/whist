from app.utils import *
from app import *
from app.logger import *
from .general import *
from .disks import *


def createVMParameters(
    vmName,
    nic_id,
    vm_size,
    location,
    operating_system="Windows",
    admin_password=None,
    admin_username=None,
):
    """Adds a vm entry to the SQL database

    Parameters:
    vmName (str): The name of the VM to add
    nic_id (str): The vm's network interface ID
    vm_size (str): The type of vm in terms of specs(default is NV6)
    location (str): The Azure region of the vm
    operating_system (str): The operating system of the vm (default is 'Windows')
    admin_password (str): The admin password (default is set in .env)

    Returns:
    dict: Parameters that will be used in Azure sdk
   """

    with engine.connect() as conn:
        oldUserNames = [
            cell[0] for cell in list(conn.execute('SELECT "username" FROM v_ms'))
        ]
        userName = genHaiku(1)[0]
        while userName in oldUserNames:
            userName = genHaiku(1)

        vm_reference = (
            {
                "publisher": "MicrosoftWindowsDesktop",
                "offer": "Windows-10",
                "sku": "rs5-pro",
                "version": "latest",
            }
            if operating_system == "Windows"
            else {
                "publisher": "Canonical",
                "offer": "UbuntuServer",
                "sku": "18.04-LTS",
                "version": "latest",
            }
        )

        print("Creating VM {} with parameters {}".format(vmName, str(vm_reference)))

        command = text(
            """
            INSERT INTO v_ms("vm_name", "disk_name")
            VALUES(:vmName, :disk_name)
            """
        )
        params = {"vmName": vmName, "username": userName, "disk_name": None}

        with engine.connect() as conn:
            conn.execute(command, **params)
            conn.close()

            admin_password = (
                os.getenv("VM_PASSWORD") if not admin_password else admin_password
            )
            admin_username = (
                os.getenv("VM_GROUP") if not admin_username else admin_username
            )

            os_profile = (
                {
                    "computer_name": vmName,
                    "admin_username": admin_username,
                    "admin_password": admin_password,
                }
                if operating_system == "Linux"
                else {
                    "computer_name": vmName,
                    "admin_username": admin_username,
                    "admin_password": admin_password,
                }
            )

            return {
                "params": {
                    "location": location,
                    "os_profile": os_profile,
                    "hardware_profile": {"vm_size": vm_size},
                    "storage_profile": {
                        "image_reference": {
                            "publisher": vm_reference["publisher"],
                            "offer": vm_reference["offer"],
                            "sku": vm_reference["sku"],
                            "version": vm_reference["version"],
                        },
                        "os_disk": {
                            "os_type": operating_system,
                            "create_option": "FromImage",
                        },
                    },
                    "network_profile": {"network_interfaces": [{"id": nic_id,}]},
                },
                "vm_name": vmName,
            }


def getVM(vm_name):
    """Retrieves information about the model view or the instance view of an Azure virtual machine

    Parameters:
    vmName (str): The name of the VM to retrieve

    Returns:
    VirtualMachine: The instance view of the virtual machine
   """
    _, compute_client, _ = createClients()
    try:
        virtual_machine = compute_client.virtual_machines.get(
            os.environ.get("VM_GROUP"), vm_name
        )
        return virtual_machine
    except:
        return None


def singleValueQuery(value):
    """Queries all vms from the v_ms SQL table with the specified name

    Parameters:
    vmName (str): The name of the VM to retrieve

    Returns:
    dict: The dictionary respresenting the query result
   """
    command = text(
        """
        SELECT * FROM v_ms WHERE "vm_name" = :value
        """
    )
    params = {"value": value}
    with engine.connect() as conn:
        exists = cleanFetchedSQL(conn.execute(command, **params).fetchall())
        conn.close()
        return True if exists else False


def updateVMUsername(username, vm_name):
    """Updates the username associated with a vm entry in the v_ms SQL table

    Parameters:
    username (str): The new username
    vm_name (str): The name of the VM row to update
   """
    command = text(
        """
        UPDATE v_ms
        SET username = :username
        WHERE
        "vm_name" = :vm_name
        """
    )
    params = {"username": username, "vm_name": vm_name}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()


def fetchVMCredentials(vm_name):
    """Fetches a vm from the v_ms sql table

    Args:
        vm_name (str): The name of the vm to fetch

    Returns:
        dict: An object respresenting the respective row in the table
    """
    command = text(
        """
        SELECT * FROM v_ms WHERE "vm_name" = :vm_name
        """
    )
    params = {"vm_name": vm_name}
    with engine.connect() as conn:
        vm_info = cleanFetchedSQL(conn.execute(command, **params).fetchone())
        conn.close()
        return vm_info


def fetchVMByIP(vm_ip):
    """Fetches a vm from the v_ms sql table

    Args:
        vm_ip (str): The ip address (ipv4) of the vm to fetch

    Returns:
        dict: An object respresenting the respective row in the table
    """
    command = text(
        """
        SELECT * FROM v_ms WHERE "ip" = :vm_ip
        """
    )
    params = {"vm_ip": vm_ip}
    with engine.connect() as conn:
        vm_info = cleanFetchedSQL(conn.execute(command, **params).fetchone())
        conn.close()
        return vm_info


def fetchVm(vm_name):
    """Fetches 1 vm from the table

    Args:
        vm_name (str): Name of vm

    Returns:
        dict: Dict for vm
    """
    command = text(
        """
        SELECT * FROM v_ms WHERE "vm_name" = :vm_name
        """
    )
    params = {"vm_name": vm_name}
    with engine.connect() as conn:
        vm_info = cleanFetchedSQL(conn.execute(command, **params).fetchone())
        conn.close()
        return vm_info


def genVMName():
    """Generates a unique name for a vm

    Returns:
        str: The generated name
    """
    with engine.connect() as conn:
        oldVMs = [cell[0] for cell in list(conn.execute('SELECT "vm_name" FROM v_ms'))]
        vmName = genHaiku(1)[0]
        while vmName in oldVMs:
            vmName = genHaiku(1)[0]
        return vmName


def fetchUserVMs(username, ID=-1):
    """Fetches vms that are connected to the user

    Args:
        username (str): The username
        ID (int, optional): The papertrail logging ID. Defaults to -1.

    Returns:
        dict: The object representing the respective vm in the v_ms sql database
    """
    sendInfo(ID, "Fetching vms for user {}".format(username))

    if username:
        command = text(
            """
            SELECT * FROM v_ms WHERE "username" = :username
            """
        )
        params = {"username": username}
        with engine.connect() as conn:
            vms_info = cleanFetchedSQL(conn.execute(command, **params).fetchall())
            conn.close()
            return vms_info
    else:
        command = text(
            """
            SELECT * FROM v_ms
            """
        )
        params = {}
        with engine.connect() as conn:
            vms_info = cleanFetchedSQL(conn.execute(command, **params).fetchall())
            conn.close()
            return vms_info


def deleteRow(vm_name, vm_names):
    """Deletes a row from the v_ms sql table, if vm_name is not in vm_names

    Args:
        vm_name (str): The name of the vm to check
        vm_names (array[string]): An array of the names of accepted VMs
    """
    if not (vm_name in vm_names):
        command = text(
            """
            DELETE FROM v_ms WHERE "vm_name" = :vm_name
            """
        )
        params = {"vm_name": vm_name}
        with engine.connect() as conn:
            conn.execute(command, **params)
            conn.close()


def insertVM(vm_name, vm_ip=None, location=None):
    """Inserts a vm into the v_ms table

    Args:
        vm_name (str): The name of the vm
        vm_ip (str, optional): The ipv4 address of the vm. Defaults to None.
        location (str, optional): The region that the vm is based in. Defaults to None.
    """
    command = text(
        """
        SELECT * FROM v_ms WHERE "vm_name" = :vm_name
        """
    )
    params = {"vm_name": vm_name}
    with engine.connect() as conn:
        vms = cleanFetchedSQL(conn.execute(command, **params).fetchall())

        if not vms:
            _, compute_client, _ = createClients()
            vm = getVM(vm_name)

            if not vm_ip or not location:
                vm_ip = getIP(vm)
                location = vm.location

            disk_name = vm.storage_profile.os_disk.name
            username = mapDiskToUser(disk_name)
            lock = False
            dev = False

            command = text(
                """
                INSERT INTO v_ms("vm_name", "username", "disk_name", "ip", "location", "lock", "dev")
                VALUES(:vm_name, :username, :disk_name, :ip, :location, :lock, :dev)
                """
            )
            params = {
                "username": username,
                "vm_name": vm_name,
                "disk_name": disk_name,
                "ip": vm_ip,
                "location": location,
                "lock": False,
                "dev": False,
            }

            conn.execute(command, **params)
            conn.close()


def updateVMIP(vm_name, ip, ID=-1):
    """Updates the ip address of a vm

    Args:
        vm_name (str): The name of the vm to update
        ip (str): The new ipv4 address
    """
    sendInfo(ID, "Updating IP for VM {} to {} in SQL".format(vm_name, ip))
    command = text(
        """
        UPDATE v_ms
        SET ip = :ip
        WHERE
        "vm_name" = :vm_name
        """
    )
    params = {"ip": ip, "vm_name": vm_name}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()


def updateVMState(vm_name, state, ID=-1):
    """Updates the state for a vm

    Args:
        vm_name (str): The name of the vm to update
        state (str): The new state of the vm
    """
    sendInfo(ID, "Updating state for VM {} to {} in SQL".format(vm_name, state, ID))
    command = text(
        """
        UPDATE v_ms
        SET state = :state
        WHERE
        "vm_name" = :vm_name
        """
    )
    params = {"vm_name": vm_name, "state": state}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()


def updateVMStateAutomatically(vm_name, ID=-1):
    """Updates the v_ms sql entry based on the azure vm state

    Args:
        vm_name (str): Name of the vm
        ID (int, optional): Papertrail Logging ID. Defaults to -1.

    Returns:
        int: 1 for success, -1 for error
    """
    _, compute_client, _ = createClients()

    vm_state = compute_client.virtual_machines.instance_view(
        resource_group_name=os.getenv("VM_GROUP"), vm_name=vm_name
    )

    hr = -1

    try:
        power_state = vm_state.statuses[1].code
        sendInfo(ID, "VM {} has Azure state {}".format(vm_name, power_state))
    except:
        sendError(ID, "VM {} Azure state unable to be fetched".format(vm_name))
    if "starting" in power_state:
        updateVMState(vm_name, "STARTING")
        sendInfo(ID, "VM {} set to state {} in Postgres".format(vm_name, "STARTING"))
        hr = 1
    elif "running" in power_state:
        updateVMState(vm_name, "RUNNING_AVAILABLE")
        sendInfo(
            ID, "VM {} set to state {} in Postgres".format(vm_name, "RUNNING_AVAILABLE")
        )
        hr = 1
    elif "stopping" in power_state:
        updateVMState(vm_name, "STOPPING")
        sendInfo(ID, "VM {} set to state {} in Postgres".format(vm_name, "STOPPING"))
        hr = 1
    elif "deallocating" in power_state:
        updateVMState(vm_name, "DEALLOCATING")
        sendInfo(
            ID, "VM {} set to state {} in Postgres".format(vm_name, "DEALLOCATING")
        )
        hr = 1
    elif "stopped" in power_state:
        updateVMState(vm_name, "STOPPED")
        sendInfo(ID, "VM {} set to state {} in Postgres".format(vm_name, "STOPPED"))
        hr = 1
    elif "deallocated" in power_state:
        updateVMState(vm_name, "DEALLOCATED")
        sendInfo(ID, "VM {} set to state {} in Postgres".format(vm_name, "DEALLOCATED"))
        hr = 1

    return hr


def updateVMLocation(vm_name, location, ID=-1):
    """Updates the location of the vm entry in the v_ms sql table

    Args:
        vm_name (str): Name of vm of interest
        location (str): The new region of the vm
    """
    sendInfo(ID, "Updating location for VM {} to {} in SQL".format(vm_name, location))
    command = text(
        """
        UPDATE v_ms
        SET location = :location
        WHERE
        "vm_name" = :vm_name
        """
    )
    params = {"vm_name": vm_name, "location": location}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()


def updateVMOS(vm_name, operating_system, ID=-1):
    """Updates the OS of the vm entry in the v_ms sql table

    Args:
        vm_name (str): Name of vm of interest
        operating_system (str): The OSof the vm
    """
    sendInfo(ID, "Updating OS for VM {} to {} in SQL".format(vm_name, operating_system))
    command = text(
        """
        UPDATE v_ms
        SET os = :operating_system
        WHERE
        "vm_name" = :vm_name
        """
    )
    params = {"vm_name": vm_name, "operating_system": operating_system}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()


def fetchAttachableVMs(state, location, ID=-1):
    """Finds all vms with specified location and state that are unlocked and not in dev mode

    Args:
        state (str): State of the vm
        location (str): Azure region to look in

    Returns:
        arr[dict]: Arrray of all vms that are attachable
    """
    command = text(
        """
        SELECT * FROM v_ms WHERE "state" = :state AND "location" = :location AND "lock" = :lock AND "dev" = :dev
        """
    )
    params = {"state": state, "location": location, "lock": False, "dev": False}

    with engine.connect() as conn:
        vms = cleanFetchedSQL(conn.execute(command, **params).fetchall())
        conn.close()
        vms = vms if vms else []
        return vms


def lockVMAndUpdate(
    vm_name, state, lock, temporary_lock, change_last_updated, verbose, ID=-1
):
    MAX_LOCK_TIME = 10

    session = Session()

    command = text(
        """
        UPDATE v_ms SET state = :state, lock = :lock
        WHERE vm_name = :vm_name
        """
    )

    if temporary_lock:
        temporary_lock = min(MAX_LOCK_TIME, temporary_lock)
        temporary_lock = shiftUnixByMinutes(dateToUnix(getToday()), temporary_lock)

        command = text(
            """
            UPDATE v_ms SET state = :state, lock = :lock, temporary_lock = :temporary_lock
            WHERE vm_name = :vm_name
            """
        )

    params = {
        "vm_name": vm_name,
        "state": state,
        "lock": lock,
        "temporary_lock": temporary_lock,
    }

    session.execute(command, params)
    session.commit()
    session.close()


def lockVM(
    vm_name,
    lock,
    username=None,
    disk_name=None,
    change_last_updated=True,
    verbose=True,
    ID=-1,
):
    """Locks/unlocks a vm. A vm entry with lock set to True prevents other processes from changing that entry.

    Args:
        vm_name (str): The name of the vm to lock
        lock (bool): True for lock
        change_last_updated (bool, optional): Whether or not to change the last_updated column as well. Defaults to True.
        verbose (bool, optional): Flag to log verbose in papertrail. Defaults to True.
        ID (int, optional): A unique papertrail logging id. Defaults to -1.
    """
    if lock and verbose:
        sendInfo(ID, "Trying to lock VM {}".format(vm_name), papertrail=verbose)
    elif not lock and verbose:
        sendInfo(ID, "Trying to unlock VM {}".format(vm_name), papertrail=verbose)

    session = Session()

    command = text(
        """
        UPDATE v_ms
        SET "lock" = :lock, "last_updated" = :last_updated
        WHERE
        "vm_name" = :vm_name
        """
    )

    if not change_last_updated:
        command = text(
            """
            UPDATE v_ms
            SET "lock" = :lock
            WHERE
            "vm_name" = :vm_name
            """
        )

    last_updated = getCurrentTime()
    params = {"vm_name": vm_name, "lock": lock, "last_updated": last_updated}

    session.execute(command, params)

    if username and disk_name:
        command = text(
            """
            UPDATE v_ms
            SET "username" = :username, "disk_name" = :disk_name
            WHERE
            "vm_name" = :vm_name
            """
        )

        params = {"username": username, "vm_name": vm_name, "disk_name": disk_name}
        session.execute(command, params)

    session.commit()
    session.close()

    if lock and verbose:
        sendInfo(ID, "Successfully locked VM {}".format(vm_name), papertrail=verbose)
    elif not lock and verbose:
        sendInfo(ID, "Successfully unlocked VM {}".format(vm_name), papertrail=verbose)


def setDev(vm_name, dev):
    command = text(
        """
        UPDATE v_ms
        SET dev = :dev
        WHERE
        "vm_name" = :vm_name
        """
    )
    params = {"dev": dev, "vm_name": vm_name}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()


def claimAvailableVM(disk_name, location, os_type="Windows", s=None, ID=-1):
    username = mapDiskToUser(disk_name)
    session = Session()

    state_preference = ["RUNNING_AVAILABLE", "STOPPED", "DEALLOCATED"]

    for state in state_preference:
        sendInfo(
            ID,
            "Looking for VMs with state {} in {} for {}".format(
                state, location, username
            ),
        )
        command = text(
            """
            SELECT * FROM v_ms
            WHERE lock = :lock AND state = :state AND dev = :dev AND os = :os_type AND location = :location
            AND (temporary_lock <= :temporary_lock OR temporary_lock IS NULL)
            """
        )
        params = {
            "lock": False,
            "state": state,
            "dev": False,
            "location": location,
            "temporary_lock": dateToUnix(getToday()),
            "os_type": os_type,
        }

        available_vm = cleanFetchedSQL(session.execute(command, params).fetchone())

        if available_vm:
            if s:
                if state == "RUNNING_AVAILABLE":
                    s.update_state(
                        state="PENDING",
                        meta={
                            "msg": "Your cloud PC is powered on. Preparing your cloud PC (this could take a few minutes)."
                        },
                    )
                else:
                    s.update_state(
                        state="PENDING",
                        meta={
                            "msg": "Your cloud PC is powered off. Preparing your cloud PC (this could take a few minutes)."
                        },
                    )
            sendInfo(
                ID,
                "Found an available VM {} for {}".format(str(available_vm), username),
            )
            command = text(
                """
                UPDATE v_ms
                SET lock = :lock, username = :username, disk_name = :disk_name, state = :state, last_updated = :last_updated
                WHERE vm_name = :vm_name
                """
            )
            params = {
                "lock": True,
                "username": username,
                "disk_name": disk_name,
                "vm_name": available_vm["vm_name"],
                "state": "ATTACHING",
                "last_updated": getCurrentTime(),
            }
            session.execute(command, params)
            sendInfo(
                ID,
                "ATTACHING VM {} to new user {}".format(
                    available_vm["vm_name"], username
                ),
            )
            session.commit()
            session.close()
            return available_vm
        else:
            sendInfo(
                ID,
                "Did not find any VMs in {} with state {} for {}.".format(
                    location, state, username
                ),
            )

    session.commit()
    session.close()
    return None


def createTemporaryLock(vm_name, minutes, ID=-1):
    """Sets the temporary lock field for a vm

    Args:
        vm_name (str): The name of the vm to temporarily lock
        minutes (int): Minutes to lock for
        ID (int, optional): Papertrail logging ID. Defaults to -1.
    """

    temporary_lock = shiftUnixByMinutes(dateToUnix(getToday()), minutes)
    session = Session()

    command = text(
        """
        UPDATE v_ms
        SET "temporary_lock" = :temporary_lock
        WHERE
        "vm_name" = :vm_name
        """
    )

    params = {"vm_name": vm_name, "temporary_lock": temporary_lock}
    session.execute(command, params)

    sendInfo(
        ID,
        "Temporary lock created for VM {} for {} minutes".format(vm_name, str(minutes)),
    )

    session.commit()
    session.close()


def vmReadyToConnect(vm_name, ready):
    """Sets the vm's ready_to_connect field

    Args:
        vm_name (str): Name of the vm
        ready (boolean): True for ready to connect
    """
    if ready:
        current = dateToUnix(getToday())
        session = Session()

        command = text(
            """
            UPDATE v_ms
            SET "ready_to_connect" = :current
            WHERE
            "vm_name" = :vm_name
            """
        )
        params = {"vm_name": vm_name, "current": current}

        session.execute(command, params)
        session.commit()
        session.close()


def checkLock(vm_name, s=None, ID=-1):
    """Check to see if a vm has been locked

    Args:
        vm_name (str): Name of the vm to check

    Returns:
        bool: True if VM is locked, False otherwise
    """
    session = Session()

    command = text(
        """
        SELECT * FROM v_ms WHERE "vm_name" = :vm_name
        """
    )
    params = {"vm_name": vm_name}

    vm = cleanFetchedSQL(session.execute(command, params).fetchone())
    session.commit()
    session.close()

    if vm:
        temporary_lock = False
        if vm["temporary_lock"]:
            temporary_lock = dateToUnix(getToday()) < vm["temporary_lock"]

            if temporary_lock and s:
                s.update_state(
                    state="PENDING",
                    meta={
                        "msg": "Estimated {} seconds remaining until update finishes.".format(
                            vm["temporary_lock"] - dateToUnix(getToday())
                        )
                    },
                )

            sendInfo(
                ID,
                "Temporary lock found on VM {}, expires at {}. It is currently {}".format(
                    vm_name, str(vm["temporary_lock"]), str(dateToUnix(getToday()))
                ),
            )
        return vm["lock"] or temporary_lock
    return None


def checkTemporaryLock(vm_name, ID=-1):
    """Check to see if a vm has been locked

    Args:
        vm_name (str): Name of the vm to check

    Returns:
        bool: True if VM is locked, False otherwise
    """
    session = Session()

    command = text(
        """
        SELECT * FROM v_ms WHERE "vm_name" = :vm_name
        """
    )
    params = {"vm_name": vm_name}

    vm = cleanFetchedSQL(session.execute(command, params).fetchone())
    session.commit()
    session.close()

    if vm:
        temporary_lock = False
        if vm["temporary_lock"]:
            temporary_lock = dateToUnix(getToday()) < vm["temporary_lock"]

        return temporary_lock
    return None


def checkDev(vm_name):
    """Checks to see if a vm is in dev mode

    Args:
        vm_name (str): Name of vm to check

    Returns:
        bool: True if vm is in dev mode, False otherwise
    """
    command = text(
        """
        SELECT * FROM v_ms WHERE "vm_name" = :vm_name
        """
    )
    params = {"vm_name": vm_name}

    with engine.connect() as conn:
        vm = cleanFetchedSQL(conn.execute(command, **params).fetchone())
        conn.close()
        if vm:
            return vm["dev"]
        return None


def checkWinlogon(vm_name):
    """Checks if a vm is ready to connect

    Args:
        vm_name (str): Name of the vm to check

    Returns:
        bool: True if vm is ready to connect
    """
    command = text(
        """
        SELECT * FROM v_ms WHERE "vm_name" = :vm_name
        """
    )
    params = {"vm_name": vm_name}

    with engine.connect() as conn:
        vm = cleanFetchedSQL(conn.execute(command, **params).fetchone())
        conn.close()
        if vm:
            return dateToUnix(getToday()) - vm["ready_to_connect"] < 10
        return None


def mapDiskToVM(disk_name):
    """Find the vm with the specified disk attached

    Args:
        disk_name (str): Name of the disk to look for

    Returns:
        dict: The vm with the specified disk attached
    """
    command = text(
        """
        SELECT * FROM v_ms WHERE "disk_name" = :disk_name
        """
    )

    params = {"disk_name": disk_name}

    with engine.connect() as conn:
        vms = cleanFetchedSQL(conn.execute(command, **params).fetchall())
        return vms


def updateVM(vm_name, location, disk_name, username):
    """Updates the vm entry in the vm_s sql table

    Args:
        vm_name (str): The name of the vm to update
        location (str): The new Azure region of the vm
        disk_name (str): The new disk that is attached to the vm
        username (str): The new username associated with the vm
    """
    command = text(
        """
        UPDATE v_ms
        SET "location" = :location, "disk_name" = :disk_name, "username" = :username
        WHERE
        "vm_name" = :vm_name
        """
    )
    params = {
        "location": location,
        "vm_name": vm_name,
        "disk_name": disk_name,
        "username": username,
    }
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()


def deleteVMFromTable(vm_name):
    """Deletes a vm from the v_ms sql table

    Args:
        vm_name (str): The name of the vm to delete
    """
    command = text(
        """
        DELETE FROM v_ms WHERE "vm_name" = :vm_name
        """
    )
    params = {"vm_name": vm_name}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()


def attachDiskToVM(disk_name, vm_name, lun, ID=-1):
    """Attach a secondary disk to a vm

    Args:
        disk_name (str): Name of the disk
        vm_name (str): Name of the vm
        lun (int): The logical unit number of the disk.  This value is used to identify data disks within the VM and therefore must be unique for each data disk attached to a VM.
        ID (int, optional): The unique papertrail logging id. Defaults to -1.

    Returns:
        int: ! for success, -1 for fail
    """
    try:
        _, compute_client, _ = createClients()
        virtual_machine = getVM(vm_name)

        data_disk = compute_client.disks.get(os.getenv("VM_GROUP"), disk_name)
        virtual_machine.storage_profile.data_disks.append(
            {
                "lun": lun,
                "name": disk_name,
                "create_option": DiskCreateOption.attach,
                "managed_disk": {"id": data_disk.id},
            }
        )

        async_disk_attach = compute_client.virtual_machines.create_or_update(
            os.getenv("VM_GROUP"), virtual_machine.name, virtual_machine
        )
        async_disk_attach.wait()
        return 1
    except Exception as e:
        sendCritical(ID, str(e))
        return -1


def getIP(vm):
    """Gets the IP address for a vm

    Args:
        vm (str): The name of the vm

    Returns:
        str: The ipv4 address
    """
    _, _, network_client = createClients()
    ni_reference = vm.network_profile.network_interfaces[0]
    ni_reference = ni_reference.id.split("/")
    ni_group = ni_reference[4]
    ni_name = ni_reference[8]

    net_interface = network_client.network_interfaces.get(ni_group, ni_name)
    ip_reference = net_interface.ip_configurations[0].public_ip_address
    ip_reference = ip_reference.id.split("/")
    ip_group = ip_reference[4]
    ip_name = ip_reference[8]

    public_ip = network_client.public_ip_addresses.get(ip_group, ip_name)
    return public_ip.ip_address


def deleteResource(name, delete_disk, ID=-1):
    """Deletes a vm

    Parameters:
    name (str): the name of the VM
    delete_disk (bool): whether or not to delete the disk attached to the vm
    ID (int): a unique identifier for the method in PaperTrail (default is -1)

    Returns:
    int: Error flag: 1 success, -1 fail

   """
    _, compute_client, network_client = createClients()
    vnetName, subnetName, ipName, nicName = (
        name + "_vnet",
        name + "_subnet",
        name + "_ip",
        name + "_nic",
    )
    hr = 1

    # get VM info based on name
    virtual_machine = getVM(name)
    os_disk_name = virtual_machine.storage_profile.os_disk.name

    # step 1, deallocate the VM
    try:
        sendInfo(ID, "Attempting to deallocate the VM {}".format(name))
        async_vm_deallocate = compute_client.virtual_machines.deallocate(
            os.getenv("VM_GROUP"), name
        )
        async_vm_deallocate.wait()
        # wait a whole minute to ensure it deallocated properly
        time.sleep(60)
        sendInfo(ID, "VM {} deallocated successfully".format(name))
    except Exception as e:
        sendError(ID, str(e))
        hr = -1

    # step 2, detach the IP
    try:
        sendInfo(ID, "Attempting to detach the IP of VM {}".format(name))
        subnet_obj = network_client.subnets.get(
            resource_group_name=os.getenv("VM_GROUP"),
            virtual_network_name=vnetName,
            subnet_name=subnetName,
        )
        # configure network interface parameters.
        params = {
            "location": virtual_machine.location,
            "ip_configurations": [
                {
                    "name": ipName,
                    "subnet": {"id": subnet_obj.id},
                    # None: Disassociate;
                    "public_ip_address": None,
                }
            ],
        }
        # use method create_or_update to update network interface configuration.
        async_ip_detach = network_client.network_interfaces.create_or_update(
            resource_group_name=os.getenv("VM_GROUP"),
            network_interface_name=nicName,
            parameters=params,
        )
        async_ip_detach.wait()
        sendInfo(ID, "IP detached from VM {} successfully".format(name))
    except Exception as e:
        sendError(ID, str(e))
        hr = -1

    # step 3, delete the VM
    try:
        sendInfo(ID, "Attempting to delete VM {}".format(name))
        async_vm_delete = compute_client.virtual_machines.delete(
            os.getenv("VM_GROUP"), name
        )
        async_vm_delete.wait()
        sendInfo(ID, "VM {} deleted successfully".format(name))
        deleteVMFromTable(name)
        sendInfo(ID, "VM {} removed from Postgres".format(name))
    except Exception as e:
        sendError(ID, str(e))
        hr = -1

    # step 4, delete the IP
    try:
        sendInfo(ID, "Attempting to delete ID from VM {}".format(name))
        async_ip_delete = network_client.public_ip_addresses.delete(
            os.getenv("VM_GROUP"), ipName
        )
        async_ip_delete.wait()
        sendInfo(ID, "IP deleted from VM {} successfully".format(name))
    except Exception as e:
        sendError(ID, str(e))
        hr = -1

    # step 4, delete the NIC
    try:
        sendInfo(ID, "Attempting to delete NIC from VM {}".format(name))
        async_nic_delete = network_client.network_interfaces.delete(
            os.getenv("VM_GROUP"), nicName
        )
        async_nic_delete.wait()
        sendInfo(ID, "NIC deleted from VM {} successfully".format(name))
    except Exception as e:
        sendError(ID, str(e))
        hr = -1

    # step 5, delete the Vnet
    try:
        sendInfo(ID, "Attempting to delete vnet from VM {}".format(name))
        async_vnet_delete = network_client.virtual_networks.delete(
            os.getenv("VM_GROUP"), vnetName
        )
        async_vnet_delete.wait()
        sendInfo(ID, "Vnet deleted from VM {} successfully".format(name))
    except Exception as e:
        sendError(ID, str(e))
        hr = -1

    if delete_disk:
        # step 6, delete the OS disk -- not needed anymore (OS disk swapping)
        try:
            sendInfo(ID, "Attempting to delete OS disk from VM {}".format(name))
            os_disk_delete = compute_client.disks.delete(
                os.getenv("VM_GROUP"), os_disk_name
            )
            os_disk_delete.wait()
            sendInfo(
                ID, "Disk {} deleted from VM {} successfully".format(os_disk_name, name)
            )
        except Exception as e:
            sendError(ID, str(e))
            hr = -1

    return hr


def createNic(name, location, tries):
    """Creates a network id

    Args:
        name (str): Name of the vm
        location (str): The azure region
        tries (int): The current number of tries

    Returns:
        dict: The network id object
    """
    _, _, network_client = createClients()
    vnetName, subnetName, ipName, nicName = (
        name + "_vnet",
        name + "_subnet",
        name + "_ip",
        name + "_nic",
    )
    try:
        async_vnet_creation = network_client.virtual_networks.create_or_update(
            os.getenv("VM_GROUP"),
            vnetName,
            {
                "location": location,
                "address_space": {"address_prefixes": ["10.0.0.0/16"]},
            },
        )
        async_vnet_creation.wait()

        # Create Subnet
        async_subnet_creation = network_client.subnets.create_or_update(
            os.getenv("VM_GROUP"),
            vnetName,
            subnetName,
            {"address_prefix": "10.0.0.0/24"},
        )
        subnet_info = async_subnet_creation.result()

        # Create public IP address
        public_ip_addess_params = {
            "location": location,
            "public_ip_allocation_method": "Static",
        }
        creation_result = network_client.public_ip_addresses.create_or_update(
            os.getenv("VM_GROUP"), ipName, public_ip_addess_params
        )

        public_ip_address = network_client.public_ip_addresses.get(
            os.getenv("VM_GROUP"), ipName
        )

        # Create NIC
        async_nic_creation = network_client.network_interfaces.create_or_update(
            os.getenv("VM_GROUP"),
            nicName,
            {
                "location": location,
                "ip_configurations": [
                    {
                        "name": ipName,
                        "public_ip_address": public_ip_address,
                        "subnet": {"id": subnet_info.id},
                    }
                ],
            },
        )

        return async_nic_creation.result()
    except Exception as e:
        if tries < 5:
            print(e)
            time.sleep(3)
            return createNic(name, location, tries + 1)
        else:
            return None


def swapdisk_name(s, disk_name, vm_name, needs_winlogon=True, ID=-1):
    """Attaches an OS disk to the VM. If the vm already has an OS disk attached, the vm will detach that and attach this one.

    Args:
        s (class): The reference to the parent's instance of self
        disk_name (str): The name of the disk
        vm_name (str): The name of the vm
        ID (int, optional): The unique papertrail logging ID. Defaults to -1.

    Returns:
        [type]: [description]
    """
    try:
        _, compute_client, _ = createClients()
        virtual_machine = getVM(vm_name)
        new_os_disk = compute_client.disks.get(os.getenv("VM_GROUP"), disk_name)

        virtual_machine.storage_profile.os_disk.managed_disk.id = new_os_disk.id
        virtual_machine.storage_profile.os_disk.name = new_os_disk.name

        s.update_state(
            state="PENDING",
            meta={
                "msg": "Uploading the necessary data to our servers. This could take a few minutes."
            },
        )

        sendInfo(ID, "Attaching disk {} to {}".format(disk_name, vm_name))

        start = time.perf_counter()
        async_disk_attach = compute_client.virtual_machines.create_or_update(
            os.getenv("VM_GROUP"), vm_name, virtual_machine
        )
        sendInfo(ID, async_disk_attach.result())
        end = time.perf_counter()
        sendInfo(
            ID,
            "Disk {} attached to VM {} in {} seconds".format(
                disk_name, vm_name, str(end - start)
            ),
        )

        s.update_state(
            state="PENDING",
            meta={
                "msg": "Data successfully uploaded to server. Forwarding boot request to cloud PC."
            },
        )

        return fractalVMStart(vm_name, True, needs_winlogon=needs_winlogon, s=s)
    except Exception as e:
        sendCritical(ID, str(e))
        return -1


def sendVMStartCommand(vm_name, needs_restart, needs_winlogon, ID=-1, s=None):
    """Starts a vm

    Args:
        vm_name (str): The name of the vm to start
        needs_restart (bool): Whether the vm needs to restart after
        ID (int, optional): Unique papertrail logging id. Defaults to -1.

    Returns:
        int: 1 for success, -1 for fail
    """
    sendInfo(ID, "Sending VM start command for vm {}".format(vm_name))

    try:

        def boot_if_necessary(vm_name, needs_restart, ID, s=s):
            _, compute_client, _ = createClients()

            power_state = "PowerState/deallocated"
            vm_state = compute_client.virtual_machines.instance_view(
                resource_group_name=os.getenv("VM_GROUP"), vm_name=vm_name
            )

            try:
                power_state = vm_state.statuses[1].code
            except Exception as e:
                sendCritical(ID, str(e))
                pass

            if "stop" in power_state or "dealloc" in power_state:
                if s:
                    s.update_state(
                        state="PENDING",
                        meta={
                            "msg": "Your cloud PC is powered off. Powering on (this could take a few minutes)."
                        },
                    )

                sendInfo(
                    ID,
                    "VM {} currently in state {}. Setting Winlogon to False".format(
                        vm_name, power_state
                    ),
                )
                vmReadyToConnect(vm_name, False)

                sendInfo(ID, "Starting VM {}".format(vm_name))
                lockVMAndUpdate(
                    vm_name,
                    "STARTING",
                    True,
                    temporary_lock=12,
                    change_last_updated=True,
                    verbose=False,
                    ID=ID,
                )

                async_vm_start = compute_client.virtual_machines.start(
                    os.environ.get("VM_GROUP"), vm_name
                )

                createTemporaryLock(vm_name, 12)

                sendInfo(ID, async_vm_start.result(timeout=180))

                if s:
                    s.update_state(
                        state="PENDING",
                        meta={"msg": "Your cloud PC was started successfully."},
                    )

                sendInfo(ID, "VM {} started successfully".format(vm_name))

                return 1

            if needs_restart:
                if s:
                    s.update_state(
                        state="PENDING",
                        meta={
                            "msg": "Your cloud PC needs to be restarted. Restarting (this will take no more than a minute)."
                        },
                    )

                sendInfo(
                    ID,
                    "VM {} needs to restart. Setting Winlogon to False".format(vm_name),
                )
                vmReadyToConnect(vm_name, False)

                lockVMAndUpdate(
                    vm_name,
                    "RESTARTING",
                    True,
                    temporary_lock=12,
                    change_last_updated=True,
                    verbose=False,
                    ID=ID,
                )

                async_vm_restart = compute_client.virtual_machines.restart(
                    os.environ.get("VM_GROUP"), vm_name
                )

                createTemporaryLock(vm_name, 12)

                sendInfo(ID, async_vm_restart.result())

                if s:
                    s.update_state(
                        state="PENDING",
                        meta={"msg": "Your cloud PC was restarted successfully."},
                    )

                sendInfo(ID, "VM {} restarted successfully".format(vm_name))

                return 1

            return 1

        def checkFirstTime(disk_name):
            session = Session()
            command = text(
                """
                SELECT * FROM disks WHERE "disk_name" = :disk_name
                """
            )
            params = {"disk_name": disk_name}

            disk_info = cleanFetchedSQL(session.execute(command, params).fetchone())

            if disk_info:
                session.commit()
                session.close()
                return disk_info["first_time"]

            session.commit()
            session.close()

            return False

        def changeFirstTime(disk_name, first_time=False):
            session = Session()
            command = text(
                """
                UPDATE disks SET "first_time" = :first_time WHERE "disk_name" = :disk_name
                """
            )
            params = {"disk_name": disk_name, "first_time": first_time}

            session.execute(command, params)
            session.commit()
            session.close()

        if s:
            s.update_state(
                state="PENDING",
                meta={"msg": "Cloud PC started executing boot request."},
            )

        disk_name = fetchVMCredentials(vm_name)["disk_name"]
        first_time = checkFirstTime(disk_name)
        num_boots = 1 if not first_time else 2

        for i in range(0, num_boots):
            if i == 1 and s:
                s.update_state(
                    state="PENDING",
                    meta={
                        "msg": "Since this is your first time logging on, we're running a few extra tests to ensure stability. Please allow a few extra minutes."
                    },
                )
                time.sleep(60)

            lockVMAndUpdate(
                vm_name,
                "ATTACHING",
                True,
                temporary_lock=None,
                change_last_updated=True,
                verbose=False,
                ID=ID,
            )

            if i == 1:
                needs_restart = True

            if s:
                s.update_state(
                    state="PENDING",
                    meta={"msg": "Cloud PC still executing boot request."},
                )

            boot_if_necessary(vm_name, needs_restart, ID)

            lockVMAndUpdate(
                vm_name,
                "RUNNING_AVAILABLE",
                False,
                temporary_lock=None,
                change_last_updated=True,
                verbose=False,
                ID=ID,
            )

            if s:
                s.update_state(
                    state="PENDING",
                    meta={
                        "msg": "Logging you into your cloud PC. This should take less than two minutes."
                    },
                )

            if needs_winlogon:
                print("Winlogon 1")
                winlogon = waitForWinlogon(vm_name, ID)
                while winlogon < 0:
                    boot_if_necessary(vm_name, True, ID)
                    if s:
                        s.update_state(
                            state="PENDING",
                            meta={
                                "msg": "Logging you into your cloud PC. This should take less than two minutes."
                            },
                        )
                    print("Winlogon 2")
                    winlogon = waitForWinlogon(vm_name, ID)

                if s:
                    s.update_state(
                        state="PENDING",
                        meta={"msg": "Logged into your cloud PC successfully."},
                    )

            if i == 1:
                print("First time! Going to boot {} times".format(str(num_boots)))

                lockVMAndUpdate(
                    vm_name,
                    "ATTACHING",
                    True,
                    temporary_lock=None,
                    change_last_updated=True,
                    verbose=False,
                    ID=ID,
                )

                if s:
                    s.update_state(
                        state="PENDING",
                        meta={
                            "msg": "Pre-installing your applications now. This could take several minutes."
                        },
                    )

                apps = fetchDiskApps(disk_name)

                installation = installApplications(vm_name, apps, s, ID)

                if installation["status"] != 200:
                    sendError(ID, "Error installing applications!")

                if s:
                    s.update_state(
                        state="PENDING",
                        meta={
                            "msg": "Running final performance checks. This will take two minutes."
                        },
                    )

                changeFirstTime(disk_name)

                time.sleep(20)

        lockVMAndUpdate(
            vm_name,
            "RUNNING_AVAILABLE",
            False,
            temporary_lock=2,
            change_last_updated=True,
            verbose=False,
            ID=ID,
        )

        return 1
    except Exception as e:
        sendCritical(ID, str(e))
        return -1


def waitForWinlogon(vm_name, ID=-1):
    """Periodically checks and sleeps until winlogon succeeds

    Args:
        vm_name (str): Name of the vm
        ID (int, optional): Unique papertrail logging id. Defaults to -1.

    Returns:
        int: 1 for success, -1 for fail
    """
    ready = checkWinlogon(vm_name)

    num_tries = 0

    if ready:
        sendInfo(ID, "VM {} has Winlogoned successfully".format(vm_name))
        return 1

    if checkDev(vm_name):
        sendInfo(
            ID,
            "VM {} is a DEV machine. Bypassing Winlogon. Sleeping for 50 seconds before returning.".format(
                vm_name
            ),
        )
        time.sleep(50)
        return 1

    while not ready:
        sendWarning(ID, "Waiting for VM {} to Winlogon".format(vm_name))
        time.sleep(5)
        ready = checkWinlogon(vm_name)
        num_tries += 1

        if num_tries > 30:
            sendError(ID, "Waited too long for winlogon. Sending failure message.")
            return -1

    sendInfo(
        ID,
        "VM {} has Winlogon successfully after {} tries".format(
            vm_name, str(num_tries)
        ),
    )

    return 1


def fractalVMStart(vm_name, needs_restart=False, needs_winlogon=os.environ.get("VM_GROUP") == "Fractal", ID=-1, s=None):
    """Bullies Azure into actually starting the vm by repeatedly calling sendVMStartCommand if necessary (big brain thoughts from Ming)

    Args:
        vm_name (str): Name of the vm to start
        needs_restart (bool, optional): Whether the vm needs to restart after. Defaults to False.
        ID (int, optional): Unique papertrail logging id. Defaults to -1.

    Returns:
        int: 1 for success, -1 for failure
    """
    sendInfo(
        ID, "Begin repeatedly calling sendVMStartCommand for vm {}".format(vm_name)
    )
    _, compute_client, _ = createClients()

    started = False
    start_attempts = 0

    # We will try to start/restart the VM and wait for it three times in total before giving up
    while not started and start_attempts < 3:
        start_command_tries = 0

        # First, send a basic start or restart command. Try six times, if it fails, give up
        if s:
            s.update_state(
                state="PENDING",
                meta={"msg": "Cloud PC successfully received boot request."},
            )

        while (
            sendVMStartCommand(vm_name, needs_restart, needs_winlogon, s=s) < 0
            and start_command_tries < 6
        ):
            time.sleep(10)
            start_command_tries += 1

        if start_command_tries >= 6:
            return -1

        wake_retries = 0

        # After the VM has been started/restarted, query the state. Try 12 times for the state to be running. If it is not running,
        # give up and go to the top of the while loop to send another start/restart command
        vm_state = compute_client.virtual_machines.instance_view(
            resource_group_name=os.getenv("VM_GROUP"), vm_name=vm_name
        )

        # Success! VM is running and ready to use
        if "running" in vm_state.statuses[1].code:
            updateVMState(vm_name, "RUNNING_AVAILABLE")
            sendInfo(ID, "Running found in status of VM {}".format(vm_name))
            started = True
            return 1

        while not "running" in vm_state.statuses[1].code and wake_retries < 12:
            sendWarning(
                ID,
                "VM state is currently in state {}, sleeping for 5 seconds and querying state again".format(
                    vm_state.statuses[1].code
                ),
            )
            time.sleep(5)
            vm_state = compute_client.virtual_machines.instance_view(
                resource_group_name=os.getenv("VM_GROUP"), vm_name=vm_name
            )

            # Success! VM is running and ready to use
            if "running" in vm_state.statuses[1].code:
                updateVMState(vm_name, "RUNNING_AVAILABLE")
                sendInfo(
                    ID,
                    "VM {} is running. State is {}".format(
                        vm_name, vm_state.statuses[1].code
                    ),
                )
                started = True
                return 1

            wake_retries += 1

        start_attempts += 1

    return -1


def stopVm(vm_name, ID=-1):
    _, compute_client, _ = createClients()
    async_vm_stop = compute_client.virtual_machines.power_off(
        resource_group_name=os.getenv("VM_GROUP"), vm_name=vm_name
    )
    updateVMState(vm_name, "STOPPING")
    async_vm_stop.wait()
    updateVMState(vm_name, "STOPPED")


def spinLock(vm_name, s=None, ID=-1):
    """Waits for vm to be unlocked

    Args:
        vm_name (str): Name of vm of interest
        ID (int, optional): Unique papertrail logging id. Defaults to -1.

    Returns:
        int: 1 = vm is unlocked, -1 = giving up
    """
    locked = checkLock(vm_name)

    num_tries = 0

    if not locked:
        sendInfo(ID, "VM {} is unlocked.".format(vm_name))
        return 1
    else:
        if s:
            s.update_state(
                state="PENDING",
                meta={
                    "msg": "Cloud PC is downloading an update. This could take a few minutes."
                },
            )

    while locked:
        sendWarning(ID, "VM {} is locked. Waiting to be unlocked.".format(vm_name))
        time.sleep(5)
        locked = checkLock(vm_name, s=s)
        num_tries += 1

        if num_tries > 40:
            sendCritical(
                ID, "FAILURE: VM {} is locked for too long. Giving up.".format(vm_name)
            )
            return -1

    if s:
        s.update_state(state="PENDING", meta={"msg": "Update successfully downloaded."})

    sendInfo(ID, "After waiting {} times, VM {} is unlocked".format(num_tries, vm_name))
    return 1


def updateProtocolVersion(vm_name, version):
    """Updates the protocol version associated with the vm. This is used for tracking updates.

    Args:
        vm_name (str): The name of the vm to update
        version (str): The id of the new version. It is based off a github commit number.
    """
    vm = getVM(vm_name)
    os_disk = vm.storage_profile.os_disk.name

    command = text(
        """
        UPDATE disks
        SET "version" = :version
        WHERE
           "disk_name" = :disk_name
    """
    )
    params = {"version": version, "disk_name": os_disk}

    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()


def fetchInstallCommand(app_name):
    """Fetches an install command from the install_commands sql table

    Args:
        app_name (str): The app name of the install command to fetch

    Returns:
        dict: An object respresenting the respective row in the table
    """
    command = text(
        """
        SELECT * FROM install_commands WHERE "app_name" = :app_name
        """
    )
    params = {"app_name": app_name}
    with engine.connect() as conn:
        install_command = cleanFetchedSQL(conn.execute(command, **params).fetchone())
        conn.close()
        return install_command


def installApplications(vm_name, apps, s=None, ID=-1):
    _, compute_client, _ = createClients()
    try:
        for app in apps:
            if s:
                s.update_state(
                    state="PENDING",
                    meta={
                        "msg": "Installing {app} onto your computer.".format(
                            app=str(app["app_name"])
                        )
                    },
                )
            sendInfo(
                ID, "Starting to install {} for VM {}".format(app["app_name"], vm_name)
            )
            install_command = fetchInstallCommand(app["app_name"])

            run_command_parameters = {
                "command_id": "RunPowerShellScript",
                "script": [install_command["command"]],
            }

            poller = compute_client.virtual_machines.run_command(
                os.environ.get("VM_GROUP"), vm_name, run_command_parameters
            )

            result = poller.result()
            sendInfo(ID, app["app_name"] + " installed to " + vm_name)
            sendInfo(ID, result.value[0].message)
    except Exception as e:
        sendError(ID, "ERROR: " + str(e))
        return {"status": 400}

    return {"status": 200}
