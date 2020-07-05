from app import *
from app.helpers.utils.azure.azure_general import *
from app.helpers.utils.azure.azure_resource_locks import *
from app.helpers.utils.azure.azure_resource_state_management import *
from app.helpers.utils.azure.azure_resource_modification import *


def swapDiskAndUpdate( disk_name, vm_name, needs_winlogon, resource_group, s=None):
    s.update_state(
        state="PENDING",
        meta={
            "msg": "Uploading the necessary data to our servers. This could take a few minutes."
        },
    )

    attachDiskToVM(disk_name, vm_name, resource_group)

    fractalVMStart(
        vm_name,
        needs_restart=True,
        needs_winlogon=needs_winlogon,
        resource_group=resource_group,
        s=s,
    )

    output = fractalSQLSelect(table_name="disks", params={"disk_name": disk_name})

    if output["rows"] and output["success"]:
        fractalSQLUpdate(
            table_name=resourceGroupToTable(resource_group),
            conditional_params={"vm_name": vm_name},
            new_params={
                "disk_name": disk_name,
                "username": output["rows"][0]["username"],
            },
        )
        
    return 1


def attachSecondaryDisks(username, vm_name, resource_group, s=None):
    output = fractalSQLSelect(
        table_name="disks",
        params={"username": username, "state": "ACTIVE", "main": False},
    )

    if output["success"] and output["rows"]:
        secondary_disks = output["rows"]

        lockVMAndUpdate(
            vm_name=vm_name,
            state="ATTACHING",
            lock=True,
            temporary_lock=None,
            resource_group=resource_group,
        )

        s.update_state(
            state="PENDING",
            meta={
                "msg": "{} extra storage hard drive(s) were found on your cloud PC. Running a few extra tests.".format(
                    len(secondary_disks)
                )
            },
        )

        for secondary_disk in secondary_disks:
            attachSecondaryDisk(
                secondary_disk["disk_name"], vm_name, resource_group=resource_group, s=s
            )

        # Lock immediately
        lockVMAndUpdate(
            vm_name=vm_name,
            state="RUNNING_AVAILABLE",
            lock=False,
            temporary_lock=1,
            resource_group=resource_group,
        )


def claimAvailableVM(
    username, disk_name, location, resource_group, os_type="Windows", s=None, ID=-1
):
    session = Session()

    state_preference = ["RUNNING_AVAILABLE", "STOPPED", "DEALLOCATED"]

    for state in state_preference:
        command = text(
            """
            SELECT * FROM {table_name}
            WHERE lock = :lock AND state = :state AND dev = :dev AND os = :os_type AND location = :location
            AND (temporary_lock <= :temporary_lock OR temporary_lock IS NULL)
            """.format(
                table_name=resourceGroupToTable(resource_group)
            )
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

            command = text(
                """
                UPDATE {table_name}
                SET lock = :lock, username = :username, disk_name = :disk_name, state = :state, last_updated = :last_updated
                WHERE vm_name = :vm_name
                """.format(
                table_name=resourceGroupToTable(resource_group)
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
            session.commit()
            session.close()
            return available_vm
        else:
            fractalLog(
                function="claimAvailableVM",
                label="username",
                logs="Did not find any VMs in {location} with state {state}.".format(
                    location=location, state=state
                ),
            )

    session.commit()
    session.close()
    return None

