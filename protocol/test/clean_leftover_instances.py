#!/usr/bin/env python3

import os
from e2e_helpers.aws_tools import terminate_or_stop_aws_instance
from e2e_helpers.aws_tools import get_boto3client

filepath = os.path.join("instances_to_clean.txt")

# Check if there is any instance we need to terminate or stop

if not os.path.exists(filepath) or not os.path.isfile(filepath):
    print("No leftover instances need to be stopped or terminated")
    exit()

# Do not stop or terminate the permanent instances protocol-integration-tester-server and protocol-integration-tester-client to prevent interfering with other jobs
ignore_instances = []
protocol_integration_tester_server = os.getenv("SERVER_INSTANCE_ID")
protocol_integration_tester_client = os.getenv("CLIENT_INSTANCE_ID")
if protocol_integration_tester_client is not None:
    ignore_instances.append(protocol_integration_tester_client)
if protocol_integration_tester_server is not None:
    ignore_instances.append(protocol_integration_tester_server)

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

    if instance_id in ignore_instances:
        continue

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
