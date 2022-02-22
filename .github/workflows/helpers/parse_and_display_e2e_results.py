#!/usr/bin/env python3

import os
import sys
import time
import boto3
import json
import subprocess
import argparse
import numpy as np
import glob
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
    "--compared-branch-names",
    nargs="*",
    help="The branches to compare the results to. Empty branch name will result in no comparisons. Passing the current branch will result in a comparison with the previous results for the same branch",
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
    default="match",
)

parser.add_argument(
    "--e2e-script-outcomes",
    nargs="+",
    help="The outcomes of the E2E testing script runs. This should be filled in automatically by Github CI",
    type=str,
    choices=[
        "success",
        "failure",
        "cancelled",
        "skipped",
    ],
    default="success",
)


if __name__ == "__main__":
    args = parser.parse_args()

    # Check if the E2E run was skipped or cancelled, in which case we skip this
    e2e_script_outcomes = args.e2e_script_outcomes
    if "success" not in e2e_script_outcomes and "failure" not in e2e_script_outcomes:
        print("E2E run was {}! No results to parse/display.".format(e2e_script_outcomes[0]))
        sys.exit(-1)

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

    current_branch_name = ""
    # In CI, the PR branch name is saved in GITHUB_REF_NAME, or in the GITHUB_HEAD_REF environment variable (in case this script is being run as part of a PR)
    b = github_ref_name.split("/")
    if len(b) != 2 or not b[0].isnumeric() or b[1] != "merge":
        current_branch_name = github_ref_name
    else:
        current_branch_name = os.getenv("GITHUB_HEAD_REF")

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

    network_conditions_matching_way = args.network_conditions_matching_way

    # Find the path to the folder with the most recent E2E protocol logs
    logs_root_dir = args.perf_logs_path
    test_start_time = ""
    if not os.path.isdir(logs_root_dir):
        print("Error, logs folder {} does not exist!".format(logs_root_dir))
        sys.exit(-1)
    # Look for the most recent logs folder created on the same day (or the day before in case the test ran right before midnight). If multiple logs folder are present, take the most recent one
    current_time = datetime.now()
    last_hour = current_time - timedelta(hours=1)

    # Convert to list
    logs_root_dirs = [logs_root_dir]

    for folder_name in sorted(os.listdir(logs_root_dirs[0]), reverse=True):
        if (
            current_time.strftime("%Y_%m_%d@") in folder_name
            or last_hour.strftime("%Y_%m_%d@") in folder_name
        ):
            logs_root_dirs.append(os.path.join(logs_root_dirs[0], folder_name))
            test_start_time = folder_name
    if len(logs_root_dirs) == 1:
        print("Error: protocol logs not found!")
        sys.exit(-1)

    # Remove duplicates but maintain order
    compared_branch_names = list(dict.fromkeys(args.compared_branch_names))

    # Remove first element
    logs_root_dirs = logs_root_dirs[1:]

    print("Found logs for the following experiments: ")
    experiments = []
    for log_dir in logs_root_dirs:
        if logs_contain_errors(log_dir):
            print("Logs from latest run in folder {} contain errors. Discarding.".format(log_dir))
            continue

        # Check if the log files with metrics are present
        client_log_file = os.path.join(log_dir, "client", "client.log")
        server_log_file = os.path.join(log_dir, "server", "server.log")

        if not os.path.isfile(client_log_file):
            print(
                "Error, client log file {} does not exist in folder {}".format(
                    client_log_file, log_dir
                )
            )
            continue
        if not os.path.isfile(server_log_file):
            print(
                "Error, server log file {} does not exist in folder {}".format(
                    server_log_file, log_dir
                )
            )
            continue

        experiment_metadata = parse_metadata(log_dir)
        network_conditions = "normal"
        if experiment_metadata and "network_conditions" in experiment_metadata:
            network_conditions = experiment_metadata["network_conditions"]

        client_metrics, server_metrics = extract_metrics(client_log_file, server_log_file)

        experiment_entry = {
            "experiment_metadata": experiment_metadata,
            "client_metrics": client_metrics,
            "server_metrics": server_metrics,
            "network_conditions": network_conditions,
        }

        experiments.append(experiment_entry)
        print("\t+ Folder: {} with network_conditions: {}".format(log_dir, network_conditions))

    for i, compared_branch_name in enumerate(compared_branch_names):
        print("Comparing to branch {}".format(compared_branch_name))
        # Create output Markdown file with comparisons to this branch
        results_file = open("streaming_e2e_test_results_{}.md".format(i), "w")
        results_file.write("## Results compared to branch {}\n".format(compared_branch_name))
        for j, experiment in enumerate(reversed(experiments)):
            results_file.write(
                "### Experiment {} - Network conditions: {}\n".format(
                    j,
                    experiment["network_conditions"]
                    if "network_conditions" in experiment
                    else "normal",
                )
            )
            if e2e_script_outcomes[j] == "failure":
                results_file.write(
                    ":bangbang::warning::red_circle: WARNING: the E2E streaming test script failed and the results below might be inaccurate! This could also be due to a server hang. :bangbang::warning::red_circle:\n\n"
                )
            # If we are looking to compare the results with the latest run on a branch, we need to download the relevant files first
            if compared_branch_name != "":
                download_latest_logs(
                    compared_branch_name,
                    datetime.strptime(test_start_time, "%Y_%m_%d@%H-%M-%S"),
                    experiment["network_conditions"],
                    network_conditions_matching_way,
                )
                compared_client_log_path = os.path.join(
                    ".", compared_branch_name, "client", "client.log"
                )
                compared_server_log_path = os.path.join(
                    ".", compared_branch_name, "server", "server.log"
                )

                compared_client_metrics = {}
                compared_server_metrics = {}
                if not os.path.isfile(compared_client_log_path) or not os.path.isfile(
                    compared_server_log_path
                ):
                    print(
                        "Could not parse {} client/server logs. Unable to compare performance results to latest {} measurements.".format(
                            compared_branch_name, compared_branch_name
                        )
                    )
                else:
                    # Extract the metric values and save them in a dictionary
                    compared_client_metrics, compared_server_metrics = extract_metrics(
                        compared_client_log_path, compared_server_log_path
                    )

                client_table_entries, server_table_entries = compute_deltas(
                    experiment["client_metrics"],
                    experiment["server_metrics"],
                    compared_client_metrics,
                    compared_server_metrics,
                )
                compared_experiment_metadata = parse_metadata(
                    os.path.join(".", compared_branch_name)
                )
                generate_comparison_table(
                    results_file,
                    most_interesting_metrics,
                    experiment["experiment_metadata"],
                    compared_experiment_metadata,
                    client_table_entries,
                    server_table_entries,
                    compared_branch_name,
                )
            else:
                generate_no_comparison_table(
                    results_file,
                    experiment["experiment_metadata"],
                    most_interesting_metrics,
                    experiment["client_metrics"],
                    experiment["server_metrics"],
                )
        results_file.write("\n\n")
        results_file.close()

    #######################################################################################

    if (
        "experiment_metadata" in experiments[0]
        and "start_time" in experiments[0]["experiment_metadata"]
    ):
        test_start_time = experiments[0]["experiment_metadata"]["start_time"]

    title = "Protocol End-to-End Streaming Test Results - {}".format(test_start_time)
    github_repo = "whisthq/whist"
    # Adding timestamp to prevent overwrite of message
    identifier = "AUTOMATED_STREAMING_E2E_TEST_RESULTS_MESSAGE"

    # Create one file for each branch

    md_files = glob.glob("*.md")
    body = []
    merged_body = ""
    for fn in md_files:
        with open(fn, "r") as f:
            contents = f.read()
            body.append((fn, contents))
            merged_body += contents

    gist_url = create_github_gist_post(github_gist_token, title, body)

    # Post updates to Slack channel if we are on dev
    if current_branch_name == "dev":
        slack_catchy_title = ":rocket::face_with_cowboy_hat::bar_chart: {} :rocket::face_with_cowboy_hat::bar_chart:".format(
            title
        )
        create_slack_post(slack_webhook, slack_catchy_title, gist_url)
    # Otherwise post on Github if the branch is tied to a open PR
    else:
        pr_number = search_open_PR(current_branch_name)
        if pr_number != -1:
            github_comment_update(
                github_token,
                github_repo,
                pr_number,
                identifier,
                merged_body,
                title=title,
                update_date=True,
            )
