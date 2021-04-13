import os

with open(os.path.join(os.path.dirname(__file__), "no_ecs_userdata.sh"), "r") as f:
    userdata_template = f.read()
