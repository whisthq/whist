import sys
import os
import pytest
import requests
from dotenv import load_dotenv
import time

load_dotenv()
SERVER_URL = "https://" + os.getenv("HEROKU_APP_NAME") + ".herokuapp.com" if os.getenv("HEROKU_APP_NAME") else "http://localhost:5000"

def getStatus(id):
    print(id)
    resp = requests.get((SERVER_URL + "/status/" + id))
    return resp.json()

def create(vm_size, location, operating_system, admin_password, input_token):
    return requests.post((SERVER_URL + '/vm/create'), json={
        "vm_size":vm_size,
        "location":location,
        "operating_system":operating_system,
        "admin_password":admin_password
    }, headers={
        "Authorization": "Bearer " + input_token
    })

def delete(vm_name, delete_disk):
    return requests.post((SERVER_URL + "/vm/delete"), json={
        "vm_name":vm_name,
	    "delete_disk":delete_disk
    })

def test_vm(input_token):
    # Testing create
    resp = create("Standard_NV6_Promo", "eastus", "Windows", 'fractal123456789.',input_token)
    id = resp.json()["ID"]
    status = "PENDING"
    while(status == "PENDING"):
        time.sleep(5)
        status = getStatus(id)["state"]
    if status != "SUCCESS":
        delete(getStatus(id)["output"]["vm_name"], True)
    assert status == "SUCCESS"

    # Test delete
    resp = delete(getStatus(id)["output"]["vm_name"], True)
    id = resp.json()["ID"]
    status = "PENDING"
    while(status == "PENDING"):
        time.sleep(5)
        status = getStatus(id)["state"]
    assert status == "SUCCESS"


