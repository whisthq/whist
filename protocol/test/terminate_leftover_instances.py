#!/usr/bin/env python3

import os
from e2e_helpers.aws_tools import terminate_or_stop_aws_instance
from e2e_helpers.aws_tools import get_boto3client

# Check if there is any instance we need to terminate
if not os.path.exists("new_instances.txt") or not os.path.isfile("new_instances.txt"):
    print("No leftover instances need to be terminated")
    exit()

print("Connecting to E2 console...")

# Connect to the E2 console
bc = get_boto3client("us-east-1")

# Terminate the instances with the instance IDs from the file
instances_file = open("new_instances.txt", "r")
for line in instances_file.readlines():
    i = line.strip()
    print("Terminating instance {} ...".format(i))
    terminate_or_stop_aws_instance(bc, i, True)


# Close and delete file
instances_file.close()
os.remove("new_instances.txt")
