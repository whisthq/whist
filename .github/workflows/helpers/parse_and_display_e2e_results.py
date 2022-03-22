#!/usr/bin/env python3

import os
import sys
import argparse
import glob
from datetime import datetime, timedelta

sys.path.append(".github/workflows/helpers")
from notifications.slack_bot import slack_post
from notifications.github_bot import github_comment_update

from protocol.e2e_streaming_test_display_helpers.table_tools import (
    network_conditions_to_readable_form,
    generate_results_table,
    generate_comparison_table,
)

from protocol.e2e_streaming_test_display_helpers.logs_tools import (
    parse_metadata,
    download_latest_logs,
    logs_contain_errors,
)

from protocol.e2e_streaming_test_display_helpers.metrics_tools import (
    extract_metrics,
)

from protocol.e2e_streaming_test_display_helpers.git_tools import (
    create_github_gist_post,
    associate_branch_to_open_pr,
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
    help="The branches to compare the results to. Empty branch name will result in no comparisons. \
    Passing the current branch will result in a comparison with the previous results for the same branch",
    type=str,
    default="",
)

parser.add_argument(
    "--network_conditions_matching_way",
    help="Whether to only compare with runs with the same network conditions (match), only with those \
    without degradation (normal_only) or don't care (do_not_care)",
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

parser.add_argument(
    "--post-results-on-slack",
    help="Whether to post the results of the experiment on Slack. This should happen for the nightly runs of \
    the experiment, but not when the experiment is run as part of a PR",
    type=str,
    choices=["false", "true"],
    default="false",
)


if __name__ == "__main__":
    # Get script arguments
    args = parser.parse_args()
    post_results_on_slack = args.post_results_on_slack == "true"
    e2e_script_outcomes = args.e2e_script_outcomes
    network_conditions_matching_way = args.network_conditions_matching_way
    logs_root_dir = args.perf_logs_path
    compared_branch_names = list(
        dict.fromkeys(args.compared_branch_names)
    )  # Remove duplicates but maintain order

    # Check if the E2E run was skipped or cancelled, in which case we don't have any data to display
    if "success" not in e2e_script_outcomes and "failure" not in e2e_script_outcomes:
        print(f"E2E run was {e2e_script_outcomes[0]}! No results to parse/display.")
        sys.exit(-1)

    # Grab environment variables of interest, and check that required ones are set
    if not os.environ.get("GITHUB_REF_NAME"):
        print(
            "GITHUB_REF_NAME is not set! If running locally, set GITHUB_REF_NAME to the name of the current git branch."
        )
        sys.exit(-1)
    if not os.environ.get("GITHUB_GIST_TOKEN") or not os.environ.get("GITHUB_TOKEN"):
        print("GITHUB_GIST_TOKEN and GITHUB_TOKEN not set. Cannot post results to Gist/GitHub!")
        sys.exit(-1)
    if not os.environ.get("GITHUB_RUN_ID"):
        print("Not running in CI, so we won't post the results on Slack!")
        if not os.environ.get("SLACK_WEBHOOK"):
            print(
                "SLACK_WEBHOOK is not set. This means we won't be able to post the results on Slack."
            )
    github_ref_name = os.environ["GITHUB_REF_NAME"]
    github_gist_token = os.environ["GITHUB_GIST_TOKEN"]
    github_token = os.environ["GITHUB_TOKEN"]
    github_run_id = os.environ.get("GITHUB_RUN_ID")
    slack_webhook = os.environ.get("SLACK_WEBHOOK")

    current_branch_name = ""
    # In CI, the PR branch name is saved in GITHUB_REF_NAME, or in the GITHUB_HEAD_REF environment variable
    # (in case this script is being run as part of a PR)
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

    # Find the path to the folder with the most recent E2E protocol logs
    test_start_time = ""
    if not os.path.isdir(logs_root_dir):
        print(f"Error, logs folder {logs_root_dir} does not exist!")
        sys.exit(-1)
    # Look for the most recent logs folder created on the same day (or the day before in case the test ran
    # right before midnight).
    current_time = datetime.now()
    last_hour = current_time - timedelta(hours=1)

    # Create list of logs dirs with logs
    logs_root_dirs = []
    for folder_name in sorted(os.listdir(logs_root_dir)):
        if (
            current_time.strftime("%Y_%m_%d@") in folder_name
            or last_hour.strftime("%Y_%m_%d@") in folder_name
        ):
            logs_root_dirs.append(os.path.join(logs_root_dir, folder_name))
            if test_start_time == "":
                test_start_time = folder_name
    if len(logs_root_dirs) == 0:
        print("Error: protocol logs not found!")
        sys.exit(-1)

    print("Found logs for the following experiments: ")
    experiments = []
    for i, log_dir in enumerate(logs_root_dirs):

        client_log_file = os.path.join(log_dir, "client", "client.log")
        server_log_file = os.path.join(log_dir, "server", "server.log")

        experiment_metadata = parse_metadata(log_dir)

        # Get network conditions, and format them in human-readable form
        network_conditions = "unknown"
        if experiment_metadata and "network_conditions" in experiment_metadata:
            network_conditions = experiment_metadata["network_conditions"]
        human_readable_network_conditions = network_conditions_to_readable_form(network_conditions)

        client_metrics = None
        server_metrics = None

        if logs_contain_errors(log_dir, verbose=True):
            print(
                f"Logs from latest run in folder {log_dir} are incomplete or contain fatal errors. Discarding."
            )
        else:
            client_metrics, server_metrics = extract_metrics(client_log_file, server_log_file)

        experiment_entry = {
            "experiment_metadata": experiment_metadata,
            "client_metrics": client_metrics,
            "server_metrics": server_metrics,
            "network_conditions": "unknown",
            "human_readable_network_conditions": "unknown",
            "outcome": e2e_script_outcomes[i],
            "dirname": os.path.basename(log_dir),
        }

        if client_metrics is not None and server_metrics is not None:
            experiment_entry["network_conditions"] = network_conditions
            experiment_entry[
                "human_readable_network_conditions"
            ] = human_readable_network_conditions

        experiments.append(experiment_entry)
        found_error = client_metrics is None or server_metrics is None
        print(
            f"\t+ Folder: {log_dir} with network_conditions: {human_readable_network_conditions}. Error: {found_error}"
        )

    # Add entries for experiments that failed or were skipped
    for i in range(len(experiments), len(e2e_script_outcomes)):
        experiment_entry = {
            "experiment_metadata": None,
            "client_metrics": None,
            "server_metrics": None,
            "network_conditions": "unknown",
            "human_readable_network_conditions": "unknown",
            "outcome": e2e_script_outcomes[i],
            "dirname": None,
        }
        experiments.append(experiment_entry)
        print("\t+ Adding empty entry for failed/skipped experiment")

    with open(f"streaming_e2e_test_results_0.md", "w") as summary_file:
        summary_file.write("### Experiments Summary\n\n")

        summary_file.write("<details>\n")
        summary_file.write("<summary>Expand Summary</summary>\n\n\n")

        for i, experiment in enumerate(experiments):
            outcome_emoji = ":white_check_mark:" if e2e_script_outcomes[i] == "success" else ":x:"
            if experiment["dirname"] is not None:
                summary_file.write(
                    f"{outcome_emoji} **Experiment {i+1}** - {experiment['human_readable_network_conditions']}. Download logs: \n```bash\naws s3 cp s3://whist-e2e-protocol-test-logs/{current_branch_name}/{experiment['dirname']}/ {experiment['dirname']}/ --recursive\n```\n"
                )
            else:
                summary_file.write(
                    f"{outcome_emoji} **Experiment {i+1}** - {experiment['human_readable_network_conditions']}. Logs not available.\n"
                )

        summary_file.write("\n\n</details>\n\n")

    for i, compared_branch_name in enumerate(compared_branch_names):
        print(f"Comparing to branch {compared_branch_name}")
        # Create output Markdown file with comparisons to this branch
        results_file = open(f"streaming_e2e_test_results_{i+1}.md", "w")
        results_file.write(f"## Results compared to branch: `{compared_branch_name}`\n")

        results_file.write("<details>\n")
        results_file.write("<summary>Expand Results</summary>\n\n")

        for j, experiment in enumerate(experiments):
            results_file.write(
                f"### Experiment {j+1} - {experiment['human_readable_network_conditions']}\n"
            )
            if experiment["outcome"] != "success":
                results_file.write(
                    f":x: WARNING: the outcome of the experiment below was: `{experiment['outcome']}` \
                    and the results below (if any) might be inaccurate!\n\n"
                )
            if experiment["client_metrics"] is None or experiment["server_metrics"] is None:
                continue
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
                        f"Could not parse {compared_branch_name} client/server logs. Unable to compare performance results \
                        to latest {compared_branch_name} measurements."
                    )
                else:
                    # Extract the metric values and save them in a dictionary
                    compared_client_metrics, compared_server_metrics = extract_metrics(
                        compared_client_log_path, compared_server_log_path
                    )

                compared_experiment_metadata = parse_metadata(
                    os.path.join(".", compared_branch_name)
                )

                generate_comparison_table(
                    results_file,
                    experiment["experiment_metadata"],
                    compared_experiment_metadata,
                    compared_branch_name,
                    most_interesting_metrics,
                    experiment["client_metrics"],
                    experiment["server_metrics"],
                    compared_client_metrics,
                    compared_server_metrics,
                )

            else:
                generate_results_table(
                    results_file,
                    experiment["experiment_metadata"],
                    most_interesting_metrics,
                    experiment["client_metrics"],
                    experiment["server_metrics"],
                )

        results_file.write("\n\n</details>\n\n")

        results_file.close()

    #######################################################################################

    if (
        "experiment_metadata" in experiments[0]
        and "start_time" in experiments[0]["experiment_metadata"]
    ):
        test_start_time = experiments[0]["experiment_metadata"]["start_time"]

    title = f"Protocol End-to-End Streaming Test Results - {test_start_time}"
    github_repo = "whisthq/whist"
    # Adding timestamp to prevent overwrite of message
    identifier = "AUTOMATED_STREAMING_E2E_TEST_RESULTS_MESSAGE"

    # Create one file for each branch

    md_files = glob.glob("streaming_e2e_test_results_*.md")
    files_list = []
    merged_files = ""
    for filename in sorted(md_files):
        with open(filename, "r") as f:
            contents = f.read()
            files_list.append((filename, contents))
            merged_files += contents

    gist_url = create_github_gist_post(github_gist_token, title, files_list)

    test_outcome = ":white_check_mark: All experiments succeeded!"
    for outcome in e2e_script_outcomes:
        if outcome != "success":
            test_outcome = ":x: " + str(outcome)

    # Post updates to Slack channel if desired
    if slack_webhook and post_results_on_slack and github_run_id:
        link_to_runner_logs = f"https://github.com/whisthq/whist/actions/runs/{github_run_id}"
        if test_outcome == ":white_check_mark: All experiments succeeded!":
            body = (
                f":white_check_mark: All E2E experiments succeeded <{link_to_runner_logs}|(see logs)>! Whist daily E2E test results available for branch: `{current_branch_name}`: {gist_url}",
            )
        else:
            body = (
                f"@releases :rotating_light: Whist daily E2E test {test_outcome} <{link_to_runner_logs}|(see logs)>! - investigate immediately: {gist_url}",
            )

        slack_post(
            slack_webhook,
            body=body,
            slack_username="Whist Bot",
            title=f":rocket::bar_chart: {title} :rocket::bar_chart:",
        )

    # Otherwise post on GitHub if the branch is tied to a open PR
    else:
        pr_number = associate_branch_to_open_pr(current_branch_name)
        if pr_number != -1:
            github_comment_update(
                github_token,
                github_repo,
                pr_number,
                identifier,
                merged_files,
                title=title,
                update_date=True,
            )
