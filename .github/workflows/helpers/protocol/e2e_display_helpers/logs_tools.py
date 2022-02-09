#!/usr/bin/env python3

import os
import sys
import time
import json
import boto3
import numpy as np
from datetime import datetime, timedelta

# add the current directory to the path no matter where this is called from
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "."))


def parse_metadata(folder_name):
    metadata_filename = os.path.join(folder_name, "experiment_metadata.json")
    experiment_metadata = None
    if not os.path.isfile(metadata_filename):
        print("Metadata file {} does not exist".format(metadata_filename))
    else:
        with open(metadata_filename, "r") as metadata_file:
            experiment_metadata = json.load(metadata_file)
    return experiment_metadata


def logs_contain_errors(logs_root_dir):
    # Check if the log files with metrics are present
    client_log_file = os.path.join(logs_root_dir, "client", "client.log")
    server_log_file = os.path.join(logs_root_dir, "server", "server.log")

    if not os.path.isfile(client_log_file) or not os.path.isfile(server_log_file):
        return True
    client_log_num_lines = sum(1 for x in open(client_log_file))
    server_log_num_lines = sum(1 for x in open(server_log_file))

    if client_log_num_lines < 500 or server_log_num_lines < 500:
        return True

    return False


def download_latest_logs(
    branch_name, before_date, network_conditions, network_conditions_matching_way
):
    client = boto3.client("s3")
    s3_resource = boto3.resource("s3")
    bucket = s3_resource.Bucket("whist-e2e-protocol-test-logs")

    if os.path.exists(branch_name):
        os.system("rm -rf {}".format(branch_name))

    os.makedirs(os.path.join(".", branch_name, "client"))
    os.makedirs(os.path.join(".", branch_name, "server"))
    compared_client_log_path = os.path.join(".", branch_name, "client", "client.log")
    compared_server_log_path = os.path.join(".", branch_name, "server", "server.log")
    exp_meta_path = os.path.join(".", branch_name, "experiment_metadata.json")

    local_timezone = int(time.timezone / 3600.0)
    before_date = before_date + timedelta(hours=local_timezone)

    result = client.list_objects(
        Bucket="whist-e2e-protocol-test-logs", Prefix="{}/".format(branch_name), Delimiter="/"
    )

    folders = result.get("CommonPrefixes")
    if folders is None:
        print("Warning, S3 does not contain logs for branch {}".format(branch_name))
        return
    counter = 1
    for folder_name in reversed(folders):
        subfolder_name = folder_name.get("Prefix").split("/")[-2]

        # Make sure that we are comparing this run to a previous run
        subfolder_date = datetime.strptime(subfolder_name, "%Y_%m_%d@%H-%M-%S")
        if subfolder_date >= before_date:
            counter += 1
            continue

        for obj in bucket.objects.filter(Prefix="{}/{}".format(branch_name, subfolder_name)):
            if "client.log" in obj.key:
                bucket.download_file(obj.key, compared_client_log_path)
            elif "server.log" in obj.key:
                bucket.download_file(obj.key, compared_server_log_path)
            elif "experiment_metadata.json" in obj.key:
                bucket.download_file(obj.key, exp_meta_path)

        # Check if logs are sane, if so stop
        if not logs_contain_errors(os.path.join(".", branch_name)):
            break
        else:
            # Get the network conditions for the compared run. If the .json file does not exist, then assume it's 'normal'
            compared_network_conditions = "normal"
            if os.path.isfile(exp_meta_path):
                compared_run_meta = parse_metadata(os.path.join(".", branch_name))
                if compared_run_meta and "network_conditions" in compared_run_meta:
                    compared_network_conditions = compared_run_meta["network_conditions"]
            # If the network conditions of the run in question are what we want, we are done. Otherwise, try with another set of logs
            if (
                (
                    network_conditions_matching_way == "match"
                    and network_conditions == compared_network_conditions
                )
                or (
                    network_conditions_matching_way == "normal_only"
                    and compared_network_conditions == "normal"
                )
                or (network_conditions_matching_way == "do_not_care")
            ):
                break

            os.system("rm -rf {}".format(compared_client_log_path))
            os.system("rm -rf {}".format(compared_server_log_path))
            counter += 1
    if counter > 1:
        print(
            "Warning, we are attempting to use {}° most recent logs from branch {}".format(
                counter, branch_name
            )
        )
