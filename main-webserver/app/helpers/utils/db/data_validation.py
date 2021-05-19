#!/usr/bin/env python
import dataclasses
from typing import List, Optional
from pydantic.dataclasses import dataclass
from flask import Flask, request
from flask_pydantic import validate
from rich import print, traceback, inspect

traceback.install()


HostRegionAWS = str
UserEmail = str
UserDPI = int
UserAccessToken = str


app = Flask("flask_pydantic_app")

# Example 1: query parameters only
@app.route("/", methods=["GET"])
@validate()
def home():
    return "great"


with app.test_client() as c:
    rv = c.get("/")
    inspect(rv)
