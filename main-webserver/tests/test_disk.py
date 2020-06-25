import sys
import os
import pytest
import requests
from dotenv import load_dotenv
import time

load_dotenv()
SERVER_URL = "https://" + os.getenv("HEROKU_APP_NAME") + ".herokuapp.com" if os.getenv("HEROKU_APP_NAME") else "http://localhost:5000"

def getStatus(id):
    resp = requests.get((SERVER_URL + "/status/" + id))
    return resp.json()

def createEmpty(disk_size, username, location):
    requests.post((SERVER_URL + "/disk/createEmpty"), json={"disk_size": disk_size,
    "username": username ,
    "location" :location})

def test_createEmpty():
    createEmpty(10,"fakefake@fake.com" , "westus2")