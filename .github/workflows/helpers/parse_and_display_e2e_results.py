#!/usr/bin/env python3

import os
import sys
import time
import boto3
import json
import subprocess
import argparse
import numpy as np
from datetime import datetime, timedelta

sys.path.append(".github/workflows/helpers")
from notifications.slack_bot import slack_post
from notifications.github_bot import github_comment_update

from protocol.e2e_display_helpers.table_tools import (
    generate_no_comparison_table,
    generate_comparison_table,
)

from protocol.e2e_display_helpers.logs_tools import (
    parse_metadata,
    download_latest_logs,
    logs_contain_errors,
)

from protocol.e2e_display_helpers.metrics_tools import (
    extract_metrics,
    add_comparison_metrics,
    compute_deltas,
)

from protocol.e2e_display_helpers.remote_tools import (
    create_github_gist_post,
    create_slack_post,
    search_open_PR,
)


DESCRIPTION = """
This script will parse and display the results of a protocol end-to-end streaming test.
Optionally, it will also include a comparison with the latest results from another branch (e.g. dev)
"""

parser = argparse.ArgumentParser(description=DESCRIPTION)
parser.add_argument(
    "--perf-logs-path",
    help="The path to the folder containing the E2E streaming logs",
    type=str,
    default=os.path.join(".", "perf_logs"),
)

parser.add_argument(
    "--compared-branch-name",
    help="The branch to compare the results to. Empty branch name will result in no comparisons. Passing the current branch will result in a comparison with the previous results for the same branch",
    type=str,
    default="",
)

parser.add_argument(
    "--network_conditions_matching_way",
    help="Whether to only compare with runs with the same network conditions (match), only with those without degradation (normal_only) or don't care (do_not_care)",
    type=str,
    choices=[
        "match",
        "normal_only",
        "do_not_care",
    ],
    default="normal_only",
)

if __name__ == "__main__":
    # Grab environmental variables of interest
    if not os.environ.get("GITHUB_REF_NAME"):
        print(
            "GITHUB_REF_NAME is not set! If running locally, set GITHUB_REF_NAME to the name of the current git branch."
        )
        sys.exit(-1)
    if not os.environ.get("GITHUB_GIST_TOKEN") or not os.environ.get("GITHUB_TOKEN"):
        print("GITHUB_GIST_TOKEN and GITHUB_TOKEN not set. Cannot post results to Gist/GitHub!")
        sys.exit(-1)
    if not os.environ.get("SLACK_WEBHOOK"):
        print("SLACK_WEBHOOK is not set. This means we won't be able to post the results on Slack.")

    github_ref_name = os.environ["GITHUB_REF_NAME"]
    github_gist_token = os.environ["GITHUB_GIST_TOKEN"]
    github_token = os.environ["GITHUB_TOKEN"]
    slack_webhook = os.environ.get("SLACK_WEBHOOK")

    args = parser.parse_args()

    # A list of metrics to display (if found) in main table
    most_interesting_metrics = {
        "VIDEO_FPS_RENDERED",
        "VIDEO_END_TO_END_LATENCY",
        "MAX_VIDEO_END_TO_END_LATENCY",
        "VIDEO_INTER_FRAME_QP",
        "MAX_VIDEO_INTER_FRAME_QP",
        "VIDEO_INTRA_FRAME_QP",
        "MAX_VIDEO_INTRA_FRAME_QP",
        "AUDIO_FPS_SKIPPED",
        "MAX_AUDIO_FPS_SKIPPED",
    }

    # Find the path to the folder with the most recent E2E protocol logs
    logs_root_dir = args.perf_logs_path
    test_time = ""
    if not os.path.isdir(logs_root_dir):
        print("Error, logs folder {} does not exist!".format(logs_root_dir))
        sys.exit(-1)
    for folder_name in sorted(os.listdir(logs_root_dir), reverse=True):
        if time.strftime("%Y_%m_%d@") in folder_name:
            logs_root_dir = os.path.join(logs_root_dir, folder_name)
            test_time = folder_name
            break
    if logs_root_dir == args.perf_logs_path:
        print("Error: protocol logs not found!")
        sys.exit(-1)

    if logs_contain_errors(logs_root_dir):
        print("Logs from latest run contains errors!")
        sys.exit(-1)

    # Check if the log files with metrics are present
    client_log_file = os.path.join(logs_root_dir, "client", "client.log")
    server_log_file = os.path.join(logs_root_dir, "server", "server.log")

    if not os.path.isfile(client_log_file):
        print("Error, client log file {} does not exist".format(client_log_file))
        sys.exit(-1)
    if not os.path.isfile(server_log_file):
        print("Error, server log file {} does not exist".format(server_log_file))
        sys.exit(-1)

    experiment_metadata = parse_metadata(logs_root_dir)
    network_conditions = "normal"
    if experiment_metadata and "network_conditions" in experiment_metadata:
        network_conditions = experiment_metadata["network_conditions"]

    network_conditions_matching_way = args.network_conditions_matching_way

    client_metrics2, server_metrics2 = extract_metrics(client_log_file, server_log_file)
    compared_client_metrics2 = {}
    compared_server_metrics2 = {}

    compared_branch_name = args.compared_branch_name

    # If we are looking to compare the results with the latest run on a branch, we need to download the relevant files first
    if compared_branch_name != "":
        download_latest_logs(
            compared_branch_name,
            datetime.strptime(test_time, "%Y_%m_%d@%H-%M-%S"),
            network_conditions,
            network_conditions_matching_way,
        )
        compared_client_log_path = os.path.join(".", compared_branch_name, "client", "client.log")
        compared_server_log_path = os.path.join(".", compared_branch_name, "server", "server.log")
        if not os.path.isfile(compared_client_log_path) or not os.path.isfile(
            compared_server_log_path
        ):
            print(
                "Could not get {} client/server logs. Unable to compare performance results to latest {} measurements.".format(
                    compared_branch_name, compared_branch_name
                )
            )
        else:
            # Extract the metric values and save them in a dictionary
            compared_client_metrics2, compared_server_metrics2 = add_comparison_metrics(
                compared_client_log_path,
                compared_server_log_path,
                compared_client_metrics2,
                compared_server_metrics2,
            )

    # Here, we parse the test results into a .info file, which can be read and displayed on the GitHub PR
    # Create output .info file
    results_file = open("streaming_e2e_test_results.info", "w+")

    # Generate the report
    if compared_branch_name == "":
        generate_no_comparison_table(
            results_file,
            experiment_metadata,
            most_interesting_metrics,
            client_metrics2,
            server_metrics2,
        )
    else:
        client_table_entries, server_table_entries = compute_deltas(
            client_metrics2, server_metrics2, compared_client_metrics2, compared_server_metrics2
        )
        compared_experiment_metadata = parse_metadata(os.path.join(".", compared_branch_name))
        generate_comparison_table(
            results_file,
            most_interesting_metrics,
            experiment_metadata,
            compared_experiment_metadata,
            client_metrics2,
            server_metrics2,
            client_table_entries,
            server_table_entries,
            compared_branch_name,
        )

    results_file.close()

    #######################################################################################

    if experiment_metadata and "start_time" in experiment_metadata:
        test_time = experiment_metadata["start_time"]

    title = "Protocol End-to-End Streaming Test Results - {}".format(test_time)
    github_repo = "whisthq/whist"
    # Adding timestamp to prevent overwrite of message
    identifier = "AUTOMATED_STREAMING_E2E_TEST_RESULTS_MESSAGE - {}".format(test_time)
    f = open("streaming_e2e_test_results.info", "r")
    body = f.read()
    f.close()

    gist_url = create_github_gist_post(github_gist_token, title, body)

    # Post updates to Slack channel if we are on dev
    if github_ref_name == "dev":
        create_slack_post(slack_webhook, title, gist_url)
    # Otherwise post on Github if the branch is tied to a open PR
    else:
        pr_number = search_open_PR()
        if pr_number != -1:
            github_comment_update(
                github_token,
                github_repo,
                pr_number,
                identifier,
                body,
                title=title,
            )
