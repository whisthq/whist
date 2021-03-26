import os

with open(os.path.join(os.path.dirname(__file__), "ec2_userdata_template.sh"), "r") as f:
    userdata_template = f.read()
