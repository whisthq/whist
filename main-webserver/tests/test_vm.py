from .helpers.tests.azure_vm import *

# Define global variable (yes, I know, but this entire test is single-threaded)

pytest.vm_name = None
pytest.disk_name = None

# Start off by deleting all the VM resources in the staging resource group, if there are any


@pytest.mark.vm_serial
def test_delete_vm_initial(input_token):
    newLine()

    def deleteVMHelper(vm):
        fractalSQLUpdate(
            table_name="v_ms",
            conditional_params={"vm_name": vm["vm_name"]},
            new_params={"lock": False, "temporary_lock": 0},
        )

        fractalLog(
            function="test_delete_vm_initial",
            label="azure_vm/delete",
            logs="Starting to delete VM {vm_name}".format(vm_name=vm["vm_name"]),
        )

        resp = deleteVM(
            vm_name=vm["vm_name"],
            delete_disk=True,
            resource_group=RESOURCE_GROUP,
            input_token=input_token,
        )

        task = queryStatus(resp, timeout=10)

        if task["status"] < 1:
            fractalLog(
                function="test_delete_vm_initial",
                label="azure_vm/delete",
                logs=task["output"],
                level=logging.ERROR,
            )
            assert False

    all_vms = fetchCurrentVMs()
    fractalJobRunner(deleteVMHelper, all_vms)

    assert True


@pytest.mark.vm_serial
def test_vm_create(input_token):
    newLine()

    # regions = ["eastus", "northcentralus", "southcentralus"]
    regions = ["eastus"]

    def createVMInRegion(region):
        fractalLog(
            function="test_vm_create",
            label="azure_vm/create",
            logs="Starting to create a VM in {region}".format(region=region),
        )

        resp = createVM(
            "Standard_NV6_Promo", region, "Windows", RESOURCE_GROUP, input_token
        )

        task = queryStatus(resp, timeout=12.5)

        if task["status"] < 1:
            fractalLog(
                function="test_vm_create",
                label="azure_vm/create",
                logs=task["output"],
                level=logging.ERROR,
            )
            assert False

    fractalJobRunner(createVMInRegion, regions)

    assert True


# def test_attach(input_token):
#     vm_name = "restlessstar701"
#     disk_name = "lingeringbase19_disk"

#     # # Test create disk from image
#     # print("Testing create disk from image...")
#     # resp = createDiskFromImage("Windows", username, "eastus", "NV6", [], input_token)
#     # id = resp.json()["ID"]
#     # print(id)
#     # status = "PENDING"
#     # while status == "PENDING" or status == "STARTED":
#     #     time.sleep(5)
#     #     status = getStatus(id)["state"]
#     # if status != "SUCCESS":
#     #     delete(getStatus(id)["output"]["vm_name"], True)
#     # assert status == "SUCCESS"
#     # disk_name = getStatus(id)["output"]["disk_name"]

#     # # Test attach disk
#     # print("Testing attach disk...")
#     resp = requests.post(
#         (SERVER_URL + "/azure_disk/attach"),
#         json={"disk_name": disk_name, "resource_group": "FractalStaging"},
#         headers={"Authorization": "Bearer " + input_token},
#     )
#     id = resp.json()["ID"]
#     print(id)
#     status = "PENDING"
#     while status == "PENDING" or status == "STARTED":
#         time.sleep(5)
#         status = getStatus(id)["state"]
#     assert status == "SUCCESS"


# # Test stop
# print("Testing stop...")
# resp = requests.post((SERVER_URL + "/vm/stopvm"), json={"vm_name": vm_name})
# id = resp.json()["ID"]
# print(id)
# status = "PENDING"
# while status == "PENDING" or status == "STARTED":
#     time.sleep(5)
#     status = getStatus(id)["state"]
# assert status == "SUCCESS"
# assert getVm(vm_name)["state"] == "STOPPED"

# # Test start
# print("Testing start...")
# resp = requests.post((SERVER_URL + "/vm/start"), json={"vm_name": vm_name,})
# id = resp.json()["ID"]
# print(id)
# status = "PENDING"
# while status == "PENDING" or status == "STARTED":
#     time.sleep(5)
#     status = getStatus(id)["state"]
# assert status == "SUCCESS"
# assert getVm(vm_name)["state"] == "RUNNING_AVAILABLE"

# # Test deallocate
# print("Testing deallocate...")
# resp = requests.post((SERVER_URL + "/vm/deallocate"), json={"vm_name": vm_name,})
# id = resp.json()["ID"]
# status = "PENDING"
# while status == "PENDING" or status == "STARTED":
#     time.sleep(5)
#     status = getStatus(id)["state"]
# assert getVm(vm_name)["state"] == "DEALLOCATED"

# # Test restart
# print("Testing restart...")
# resp = requests.post((SERVER_URL + "/vm/restart"), json={"username": username})
# id = resp.json()["ID"]
# status = "PENDING"
# while(status == "PENDING" or status == "STARTED"):
#     time.sleep(5)
#     status = getStatus(id)["state"]
# assert status == "SUCCESS"

# # Test swap disk
# print("Testing swap disk...")
# resp = createDiskFromImage(
#     "Windows", username, "eastus", "NV6", [], input_token
# )
# id = resp.json()["ID"]
# status = "PENDING"
# while status == "PENDING" or status == "STARTED":
#     time.sleep(5)
#     status = getStatus(id)["state"]
# disk_name2 = getStatus(id)["output"]["disk_name"]
# resp = swap(vm_name, disk_name2, input_token)
# id = resp.json()["ID"]
# status = "PENDING"
# while status == "PENDING" or status == "STARTED":
#     time.sleep(5)
#     status = getStatus(id)["state"]
# assert getVm(vm_name)["disk_name"] == disk_name2

## Test add disk
# print("Testing add disk...")
# resp = requests.post(
#     (SERVER_URL + "/disk/createEmpty"),
#     json={"disk_size": 10, "username": username},
#     headers={"Authorization": "Bearer " + input_token}
# )
# id = resp.json()["ID"]
# status = "PENDING"
# while status == "PENDING" or status == "STARTED":
#     time.sleep(5)
#     status = getStatus(id)["state"]
# disk_name3 = getStatus(id)["output"]
# resp = requests.post(
#     (SERVER_URL + "/disk/add"),
#     json={"disk_name": disk_name3, "vm_name": vm_name},
#     headers={"Authorization": "Bearer " + input_token}
# )
# id = resp.json()["ID"]
# status = "PENDING"
# while status == "PENDING" or status == "STARTED":
#     time.sleep(5)
#     status = getStatus(id)["state"]

# # Test delete
# print("Testing delete...")
# resp = delete(vm_name, False)
# id = resp.json()["ID"]
# status = "PENDING"
# while status == "PENDING" or status == "STARTED":
#     time.sleep(5)
#     status = getStatus(id)["state"]
# assert status == "SUCCESS"

# # Delete disks
