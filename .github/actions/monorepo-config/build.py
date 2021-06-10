#!/usr/bin/env python
import os
import sys
import json
import rich

rich.traceback.install()

script = ".github/actions/monorepo-config/main.py"

json1 = json.dumps({"ICON_FILE_NAME": "neil"})
json2 = json.dumps({"NODEJS": "neil"})

args1 = [
    script,
    "-p",
    "mac",
    "-p",
    "staging",
    "-s",
    json1,
    "-s",
    json2,
    # "--out",
    # "../config-mac-staging.json",
]

args2 = [script, "--help"]

processes = [
    [],
    [],
    [],
    [],
]

os.execl(sys.executable, "python", *args2)
