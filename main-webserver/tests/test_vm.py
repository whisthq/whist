import sys
import os
import pytest
import requests
from dotenv import load_dotenv
import time

load_dotenv()
SERVER_URL = (
    "https://" + os.getenv("HEROKU_APP_NAME") + ".herokuapp.com"
    if os.getenv("HEROKU_APP_NAME")
    else "http://localhost:5000"
)


def getStatus(id):
    resp = requests.get((SERVER_URL + "/status/" + id))
    return resp.json()


def getVm(vm_name):
    resp = requests.post((SERVER_URL + "/vm/fetchVm"), json={"vm_name": vm_name})
    return resp.json()["vm"]


def create(vm_size, location, operating_system, admin_password, input_token):
    return requests.post(
        (SERVER_URL + "/vm/create"),
        json={
            "vm_size": vm_size,
            "location": location,
            "operating_system": operating_system,
            "admin_password": admin_password,
        },
        headers={"Authorization": "Bearer " + input_token},
    )


def delete(vm_name, delete_disk):
    return requests.post(
        (SERVER_URL + "/vm/delete"),
        json={"vm_name": vm_name, "delete_disk": delete_disk},
    )


def createDiskFromImage(
    operating_system, username, location, vm_size, apps, input_token
):
    return requests.post(
        (SERVER_URL + "/disk/createFromImage"),
        json={
            "operating_system": operating_system,
            "username": username,
            "location": location,
            "vm_size": vm_size,
            "apps": apps,
            "resource_group": os.getenv("VM_GROUP"),
        },
        headers={"Authorization": "Bearer " + input_token},
    )

def swap(vm_name, disk_name, input_token):
    return requests.post((SERVER_URL + "/disk/swap"), json={"vm_name": vm_name, "disk_name": disk_name},headers={"Authorization": "Bearer " + input_token},)

def test_vm(input_token):
    username = "fakefake@delete.com"

    # Testing create
    print("Testing create vm...")
    resp = create(
        "Standard_NV6_Promo", "eastus", "Windows", "fractal123456789.", input_token
    )
    id = resp.json()["ID"]
    print("ID: " + id)
    status = "PENDING"
    while status == "PENDING" or status == "STARTED":
        time.sleep(5)
        status = getStatus(id)["state"]
    if status != "SUCCESS":
        delete(getStatus(id)["output"]["vm_name"], True)
    assert status == "SUCCESS"
    vm_name = getStatus(id)["output"]["vm_name"]

    # Test create disk from image
    print("Testing create disk from image...")
    resp = createDiskFromImage(
        "Windows", username, "eastus", "NV6", [], input_token
    )
    id = resp.json()["ID"]
    status = "PENDING"
    while status == "PENDING" or status == "STARTED":
        time.sleep(5)
        status = getStatus(id)["state"]
    if status != "SUCCESS":
        delete(getStatus(id)["output"]["vm_name"], True)
    assert status == "SUCCESS"
    disk_name = getStatus(id)["output"]["disk_name"]

    # Test attach disk
    print("Testing attach disk...")
    resp = requests.post(
        (SERVER_URL + "/disk/attachSpecific"),
        json={"disk_name": disk_name, "vm_name": vm_name},
        headers={"Authorization": "Bearer " + input_token},
    )
    id = resp.json()["ID"]
    while status == "PENDING" or status == "STARTED":
        time.sleep(5)
        status = getStatus(id)["state"]
    assert status == "SUCCESS"

    # Test stop
    print("Testing stop...")
    requests.post((SERVER_URL + "/vm/stopvm"), json={"vm_name": vm_name})
    id = resp.json()["ID"]
    status = "PENDING"
    while(status == "PENDING" or status == "STARTED"):
        time.sleep(5)
        status = getStatus(id)["state"]
    assert status == "SUCCESS"
    assert getVm(vm_name)["state"] == "STOPPED"

    # Test start
    print("Testing start...")
    requests.post((SERVER_URL + "/vm/setDev"), json={"vm_name": vm_name, "dev": True})
    resp = requests.post((SERVER_URL + '/vm/start'), json={
        "vm_name":vm_name,
    })
    id = resp.json()["ID"]
    status = "PENDING"
    while(status == "PENDING" or status == "STARTED"):
        time.sleep(5)
        status = getStatus(id)["state"]
    assert getVm(vm_name)["state"] == "RUNNING_AVAILABLE"
    requests.post((SERVER_URL + "/vm/setDev"), json={"vm_name": vm_name, "dev": False})

    # Test deallocate
    print("Testing deallocate...")
    resp = requests.post((SERVER_URL + '/vm/deallocate'), json={
        "vm_name":vm_name,
    })
    id = resp.json()["ID"]
    status = "PENDING"
    while(status == "PENDING" or status == "STARTED"):
        time.sleep(5)
        status = getStatus(id)["state"]
    assert getVm(vm_name)["state"] == "DEALLOCATED"

    # Test restart
    print("Testing restart...")
    resp = requests.post((SERVER_URL + "/vm/restart"), json={"username": username})
    id = resp.json()["ID"]
    status = "PENDING"
    while(status == "PENDING" or status == "STARTED"):
        time.sleep(5)
        status = getStatus(id)["state"]
    assert status == "SUCCESS"

    # Test swap disk
    print("Testing swap disk...")
    resp = createDiskFromImage(
        "Windows", username, "eastus", "NV6", [], input_token
    )
    id = resp.json()["ID"]
    status = "PENDING"
    while status == "PENDING" or status == "STARTED":
        time.sleep(5)
        status = getStatus(id)["state"]
    disk_name2 = getStatus(id)["output"]["disk_name"]
    resp = swap(vm_name, disk_name2, input_token)
    id = resp.json()["ID"]
    status = "PENDING"
    while status == "PENDING" or status == "STARTED":
        time.sleep(5)
        status = getStatus(id)["state"]
    assert getVm(vm_name)["disk_name"] == disk_name2

    # Test add disk
    print("Testing add disk...")

    # Test delete
    print("Testing delete...")
    resp = delete(vm_name, True)
    id = resp.json()["ID"]
    status = "PENDING"
    while status == "PENDING" or status == "STARTED":
        time.sleep(5)
        status = getStatus(id)["state"]
    assert status == "SUCCESS"
