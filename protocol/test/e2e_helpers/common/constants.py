#!/usr/bin/env python3

import os

# The Github job name (or None if running locally)
job_name = os.getenv("GITHUB_JOB")
# Name of configs file to use
configs_filename = (
    "e2e.yaml"
    if job_name == "protocol-streaming-e2e-check-pr"
    else "backend_integration.yaml"
    if job_name == "backend-integration-test"
    else "default.yaml"
)
# The name to use to tag any new AWS EC2 instance that is created. A suffix with the branch name will be added.
instances_name_tag = (
    "protocol-e2e-benchmarking"
    if job_name == "protocol-streaming-e2e-check-pr"
    else "backend-integration-test"
    if job_name == "backend-integration-test"
    else "manual-e2e-test"
)
github_run_id = os.getenv("GITHUB_RUN_ID") or "personal machine"

# The expected length of the Github SHA string
GITHUB_SHA_LEN = 40

# The username to use to access the AWS EC2 instance(s)
username = "ubuntu"
# The number of times to retry if a SSH connection is refused or if the connection attempt times out
ssh_connection_retries = 5
# The timeout after which we give up on commands that have not finished on a remote AWS EC2 instance.
# This value should not be set to less than 40mins (2400s). For the backend test, we set it to a larger
# number as sometimes building the mandelboxes takes longer if the network is slow
aws_timeout_seconds = 2400 if job_name != "backend-integration-test" else 3600

# The path (on the machine running this script) to the file containing the AWS credentials to use
# to access the Whist AWS console. The file should contain the access key ID and the secret access key.
# It is created/updated when running `aws configure`"
aws_credentials_filepath = os.path.join(os.path.expanduser("~"), ".aws", "credentials")

pexpect_max_retries = 5

# Max number of times to retry setup commands that can fail due to API outages
SETUP_MAX_RETRIES = 5
# The timeout after which we give up finishing the host setup
HOST_SETUP_TIMEOUT_SECONDS = 5 * 60  # 5 mins

# The linux exit codes for timeout errors
TIMEOUT_EXIT_CODE = 124
TIMEOUT_KILL_EXIT_CODE = 137

# Number of network condition parameters that are supported
N_NETWORK_CONDITION_PARAMETERS = 5

# The expected length of the protocol session ID string
SESSION_ID_LEN = 13

# Max number of times to retry building the client / server mandelboxes (which can fail due to API outages)
MANDELBOX_BUILD_MAX_RETRIES = 5

# Whether the E2E script is running in CI vs on a local machine
running_in_ci = os.getenv("CI") == "true"
