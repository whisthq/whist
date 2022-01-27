#!/usr/bin/env python3

import os
from aws_tools import terminate_or_stop_aws_instance
from aws_tools import get_boto3client

filepath = os.path.join("instances_to_clean.txt")

# Check if there is any instance we need to terminate or stop

if not os.path.exists(filepath) or not os.path.isfile(filepath):
    print("No leftover instances need to be stopped or terminated")
    exit()

# Terminate or stop the instances with the instance IDs from the file.
# We terminate instances that were created anew by the
# streaming_e2e_tester.py script. We stop instances that were reused by
# the streaming_e2e_tester.py script.
instances_file = open(filepath, "r")
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
os.remove(filepath)
