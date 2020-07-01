# from app import *
# from app.helpers.utils.azure.azure_general import *
# from app.helpers.utils.azure.azure_resource_locks import *
# from app.helpers.utils.azure.azure_resource_state_management import *


# def swapDiskAndUpdate(s, disk_name, vm_name, needs_winlogon, resource_group):
#     s.update_state(
#         state="PENDING",
#         meta={
#             "msg": "Uploading the necessary data to our servers. This could take a few minutes."
#         },
#     )

#     attachDiskToVM(disk_name, vm_name, resource_group)

#     fractalVMStart(
#         vm_name,
#         needs_restart=True,
#         needs_winlogon=needs_winlogon,
#         resource_group=resource_group,
#         s=s,
#     )

#     output = fractalSQLSelect(table_name="disks", params={"disk_name": disk_name})

#     if output["rows"] and output["success"]:
#         fractalSQLUpdate(
#             table_name=resourceGroupToTable(resource_group),
#             conditional_params={"vm_name": vm_name},
#             new_params={
#                 "disk_name": disk_name,
#                 "username": output["rows"][0]["username"],
#             },
#         )

# def attachSecondaryDisks(s, username, vm_name):
#     output = fractalSQLSelect(table_name="disks", params={
#         "username": username,
#         "state": "ACTIVE",
#         "main": False
#     })

#     if output["success"] and output["rows"]:
#         secondary_disks = output["rows"]

#         lockVMAndUpdate(
#             vm_name=vm_name,
#             state="ATTACHING",
#             lock=True,
#             temporary_lock=None
#         )

#         s.update_state(
#             state="PENDING",
#             meta={
#                 "msg": "{} extra storage hard drive(s) were found on your cloud PC. Running a few extra tests.".format(
#                     len(secondary_disks)
#                 )
#             },
#         )

#         for secondary_disk in secondary_disks:
#             attachDisk(secondary_disk["disk_name"], vm_name)

#         # Lock immediately
#         lockVMAndUpdate(
#             vm_name=vm_name,
#             state="RUNNING_AVAILABLE",
#             lock=False,
#             temporary_lock=1,
#             change_last_updated=True,
#             verbose=False,
#             ID=ID,
#         )
