#!/usr/bin/env python3

import os
from e2e_helpers.aws_tools import terminate_or_stop_aws_instance
from e2e_helpers.aws_tools import get_boto3client

# Check if there is any instance we need to terminate
if not os.path.exists("instances_to_clean.txt") or not os.path.isfile("instances_to_clean.txt"):
    print("No leftover instances need to be terminated")
    exit()

print("Connecting to E2 console...")

# Terminate or stop the instances with the instance IDs from the file
instances_file = open("instances_to_clean.txt", "r")
for line in instances_file.readlines():
    i = line.strip().split()
    action = i[0]
    region = i[1]
    instance_id = i[2]

    # Connect to the E2 console
    bc = get_boto3client(region)
    should_terminate = True if action == "terminate" else False
    print(
        "{} instance {} on region {} ...".format(
            "Terminating" if should_terminate else "Stopping", instance_id, region
        )
    )
    terminate_or_stop_aws_instance(bc, instance_id, should_terminate)


# Close and delete file
instances_file.close()
os.remove("instances_to_clean.txt")
