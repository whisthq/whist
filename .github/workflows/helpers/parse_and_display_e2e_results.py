import os
import sys
import time
import boto3
import json
import subprocess
import argparse
import numpy as np
from pytablewriter import MarkdownTableWriter
from contextlib import redirect_stdout
from github import Github, InputFileContent

sys.path.append(".github/workflows/helpers")
from notifications.slack_bot import slack_post
from notifications.github_bot import github_comment_update


# Extract the metric values and save them in two dictionaries
def extract_metrics(client_log_file, server_log_file):
    client_metrics = {}
    server_metrics = {}

    with open(client_log_file, "r") as f:
        for line in f.readlines():
            if "METRIC" in line:
                l = line.strip().split()
                metric_name = l[-3].strip('"')
                metric_value = float(l[-1].strip('"'))
                if metric_name not in client_metrics:
                    client_metrics[metric_name] = [metric_value]
                else:
                    client_metrics[metric_name].append(metric_value)

    with open(server_log_file, "r") as f:
        for line in f.readlines():
            if "METRIC" in line:
                l = line.strip().split()
                metric_name = l[-3].strip('"')
                metric_value = float(l[-1].strip('"'))
                if metric_name not in server_metrics:
                    server_metrics[metric_name] = [metric_value]
                else:
                    server_metrics[metric_name].append(metric_value)

    client_metrics2 = {}
    server_metrics2 = {}

    for k in client_metrics:
        client_metrics[k] = np.array(client_metrics[k])
        client_metrics2[k] = {
            "entries": len(client_metrics[k]),
            "avg": np.mean(client_metrics[k]),
            "std": np.std(client_metrics[k]),
            "max": np.max(client_metrics[k]),
            "min": np.min(client_metrics[k]),
        }

    for k in server_metrics:
        server_metrics[k] = np.array(server_metrics[k])
        server_metrics2[k] = {
            "entries": len(server_metrics[k]),
            "avg": np.mean(server_metrics[k]),
            "std": np.std(server_metrics[k]),
            "max": np.max(server_metrics[k]),
            "min": np.min(server_metrics[k]),
        }

    return client_metrics2, server_metrics2


# Extract metric values for the client-server pair to compare to current one, and add results to existing dictionaries
def add_comparison_metrics(
    compared_client_log_file, compared_server_log_file, client_dictionary, server_dictionary
):
    compared_client_metrics = {}
    compared_server_metrics = {}

    with open(compared_client_log_file, "r") as f:
        for line in f.readlines():
            if "METRIC" in line:
                l = line.strip().split()
                metric_name = l[-3].strip('"')
                metric_value = float(l[-1].strip('"'))
                if metric_name not in compared_client_metrics:
                    compared_client_metrics[metric_name] = [metric_value]
                else:
                    compared_client_metrics[metric_name].append(metric_value)

    with open(compared_server_log_file, "r") as f:
        for line in f.readlines():
            if "METRIC" in line:
                l = line.strip().split()
                metric_name = l[-3].strip('"')
                metric_value = float(l[-1].strip('"'))
                if metric_name not in compared_server_metrics:
                    compared_server_metrics[metric_name] = [metric_value]
                else:
                    compared_server_metrics[metric_name].append(metric_value)

    for k in compared_client_metrics:
        compared_client_metrics[k] = np.array(compared_client_metrics[k])
        client_dictionary[k] = {
            "entries": len(compared_client_metrics[k]),
            "avg": np.mean(compared_client_metrics[k]),
            "std": np.std(compared_client_metrics[k]),
            "max": np.max(compared_client_metrics[k]),
            "min": np.min(compared_client_metrics[k]),
        }

    for k in compared_server_metrics:
        compared_server_metrics[k] = np.array(compared_server_metrics[k])
        server_dictionary[k] = {
            "entries": len(compared_server_metrics[k]),
            "avg": np.mean(compared_server_metrics[k]),
            "std": np.std(compared_server_metrics[k]),
            "max": np.max(compared_server_metrics[k]),
            "min": np.min(compared_server_metrics[k]),
        }

    return client_dictionary, server_dictionary


def compute_deltas(
    client_dictionary, server_dictionary, compared_client_dictionary, compared_server_dictionary
):
    # Augment dictionaries with deltas wrt to other result, if available
    for k in client_dictionary:
        if k in compared_client_dictionary:
            client_dictionary[k]["compared_entries"] = compared_client_dictionary[k]["entries"]
            client_dictionary[k]["compared_avg"] = round(compared_client_dictionary[k]["avg"], 3)
            client_dictionary[k]["compared_std"] = round(compared_client_dictionary[k]["std"], 3)
            client_dictionary[k]["delta"] = round(
                client_dictionary[k]["avg"] - client_dictionary[k]["compared_avg"], 3
            )
            if client_dictionary[k]["delta"] == 0:
                client_dictionary[k]["delta"] = "-"
                client_dictionary[k]["delta_pctg"] = "-"
            elif client_dictionary[k]["compared_avg"] == 0:
                client_dictionary[k]["delta_pctg"] = "nan"
            else:
                client_dictionary[k]["delta_pctg"] = round(
                    (client_dictionary[k]["delta"] / client_dictionary[k]["compared_avg"]), 3
                )
        else:
            client_dictionary[k]["compared_avg"] = "N/A"
            client_dictionary[k]["compared_std"] = "N/A"
            client_dictionary[k]["delta"] = "N/A"
            client_dictionary[k]["delta_pctg"] = "N/A"

    # Create client table entries with the desired format
    client_table_entries = []
    for k in client_dictionary:
        new_entry = [k, client_dictionary[k]["entries"]]

        avg_stdv_this_branch = ""
        if client_dictionary[k]["entries"] > 1:
            avg_stdv_this_branch = "{:.3f} ± {:.3f}".format(
                client_dictionary[k]["avg"], client_dictionary[k]["std"]
            )
        else:
            avg_stdv_this_branch = "{:.3f}".format(client_dictionary[k]["avg"])

        new_entry.append(avg_stdv_this_branch)

        avg_stdv_dev = ""
        if (
            client_dictionary[k]["compared_avg"] == "N/A"
            or client_dictionary[k]["compared_std"] == "N/A"
        ):
            avg_stdv_dev = "N/A"
        elif client_dictionary[k]["compared_entries"] > 1:
            avg_stdv_dev = "{:.3f} ± {:.3f}".format(
                client_dictionary[k]["compared_avg"], client_dictionary[k]["compared_std"]
            )
        else:
            avg_stdv_dev = "{:.3f}".format(client_dictionary[k]["compared_avg"])

        new_entry.append(avg_stdv_dev)

        delta_formatted = client_dictionary[k]["delta"]
        if client_dictionary[k]["delta"] != "-" and client_dictionary[k]["delta"] != "N/A":
            delta_formatted = "{:.3f}".format(delta_formatted)
            delta_pctg_formatted = client_dictionary[k]["delta_pctg"]
            if (
                client_dictionary[k]["delta_pctg"] != "-"
                and client_dictionary[k]["delta_pctg"] != "nan"
                and client_dictionary[k]["delta_pctg"] != "N/A"
            ):
                delta_pctg_formatted = "{:.3f}".format(delta_pctg_formatted * 100.0)
            new_entry.append("{} ({}%)".format(delta_formatted, delta_pctg_formatted))
        else:
            new_entry.append(delta_formatted)

        emoji_delta = ""
        if (
            client_dictionary[k]["delta"] != "-"
            and client_dictionary[k]["delta"] != "N/A"
            and client_dictionary[k]["delta_pctg"] != "-"
            and client_dictionary[k]["delta_pctg"] != "nan"
            and client_dictionary[k]["delta_pctg"] != "N/A"
        ):
            if client_dictionary[k]["delta"] > 0:
                emoji_delta = "⬆️"
            else:
                emoji_delta = "⬇️"

        new_entry.append(emoji_delta)
        client_table_entries.append(new_entry)

    for k in server_dictionary:
        if k in compared_server_dictionary:
            server_dictionary[k]["compared_entries"] = compared_server_dictionary[k]["entries"]
            server_dictionary[k]["compared_avg"] = round(compared_server_dictionary[k]["avg"], 3)
            server_dictionary[k]["compared_std"] = round(compared_server_dictionary[k]["std"], 3)
            server_dictionary[k]["delta"] = round(
                server_dictionary[k]["avg"] - server_dictionary[k]["compared_avg"], 3
            )
            if server_dictionary[k]["delta"] == 0:
                server_dictionary[k]["delta"] = "-"
                server_dictionary[k]["delta_pctg"] = "-"
            elif server_dictionary[k]["compared_avg"] == 0:
                server_dictionary[k]["delta_pctg"] = "nan"
            else:
                server_dictionary[k]["delta_pctg"] = round(
                    (server_dictionary[k]["delta"] / server_dictionary[k]["compared_avg"]), 3
                )
        else:
            server_dictionary[k]["compared_avg"] = "N/A"
            server_dictionary[k]["compared_std"] = "N/A"
            server_dictionary[k]["delta"] = "N/A"
            server_dictionary[k]["delta_pctg"] = "N/A"

    # Create server table entries with the desired format
    server_table_entries = []
    for k in server_dictionary:
        new_entry = [k, server_dictionary[k]["entries"]]

        avg_stdv_this_branch = ""
        if server_dictionary[k]["entries"] > 1:
            avg_stdv_this_branch = "{:.3f} ± {:.3f}".format(
                server_dictionary[k]["avg"], server_dictionary[k]["std"]
            )
        else:
            avg_stdv_this_branch = "{:.3f}".format(server_dictionary[k]["avg"])

        new_entry.append(avg_stdv_this_branch)

        avg_stdv_dev = ""
        if (
            server_dictionary[k]["compared_avg"] == "N/A"
            or server_dictionary[k]["compared_std"] == "N/A"
        ):
            avg_stdv_dev = "N/A"
        elif server_dictionary[k]["compared_entries"] > 1:
            avg_stdv_dev = "{:.3f} ± {:.3f}".format(
                server_dictionary[k]["compared_avg"], server_dictionary[k]["compared_std"]
            )
        else:
            avg_stdv_dev = "{:.3f}".format(server_dictionary[k]["compared_avg"])

        new_entry.append(avg_stdv_dev)

        delta_formatted = server_dictionary[k]["delta"]
        if server_dictionary[k]["delta"] != "-" and server_dictionary[k]["delta"] != "N/A":
            delta_formatted = "{:.3f}".format(delta_formatted)
            delta_pctg_formatted = server_dictionary[k]["delta_pctg"]
            if (
                server_dictionary[k]["delta_pctg"] != "-"
                and server_dictionary[k]["delta_pctg"] != "nan"
                and server_dictionary[k]["delta_pctg"] != "N/A"
            ):
                delta_pctg_formatted = "{:.3f}".format(delta_pctg_formatted * 100.0)
            new_entry.append("{} ({}%)".format(delta_formatted, delta_pctg_formatted))
        else:
            new_entry.append(delta_formatted)

        emoji_delta = ""
        if (
            server_dictionary[k]["delta"] != "-"
            and server_dictionary[k]["delta"] != "N/A"
            and server_dictionary[k]["delta_pctg"] != "-"
            and server_dictionary[k]["delta_pctg"] != "nan"
            and server_dictionary[k]["delta_pctg"] != "N/A"
        ):
            if server_dictionary[k]["delta"] > 0:
                emoji_delta = "⬆️"
            else:
                emoji_delta = "⬇️"

        new_entry.append(emoji_delta)
        server_table_entries.append(new_entry)

    return client_table_entries, server_table_entries


def download_latest_logs(branch_name):
    client = boto3.client("s3")
    s3_resource = boto3.resource("s3")
    bucket = s3_resource.Bucket("whist-e2e-protocol-test-logs")

    if os.path.exists(branch_name):
        os.system("rm -rf {}".format(branch_name))

    os.makedirs(os.path.join(".", "{}".format(branch_name), "client"))
    os.makedirs(os.path.join(".", "{}".format(branch_name), "server"))
    compared_client_log_path = os.path.join(".", "{}".format(branch_name), "client", "client.log")
    compared_server_log_path = os.path.join(".", "{}".format(branch_name), "server", "server.log")

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

        for obj in bucket.objects.filter(
            Prefix="{}/{}".format("{}".format(branch_name), subfolder_name)
        ):
            if "client.log" in obj.key:
                bucket.download_file(obj.key, compared_client_log_path)
            elif "server.log" in obj.key:
                bucket.download_file(obj.key, compared_server_log_path)

        # Check if logs are sane, if so stop
        if not logs_contain_errors(os.path.join(".", "{}".format(branch_name))):
            break
        else:
            os.system("rm -rf {}".format(compared_client_log_path))
            os.system("rm -rf {}".format(compared_server_log_path))
            counter += 1
    if counter > 1:
        print(
            "Warning, we are attempting to use {}° most recent logs from branch {}".format(
                counter, branch_name
            )
        )


def parse_metadata(folder_name):
    metadata_filename = os.path.join(folder_name, "experiment_metadata.json")
    experiment_metadata = None
    if not os.path.isfile(metadata_filename):
        print("Metadata file {} does not exist".format(metadata_filename))
    else:
        with open(metadata_filename, "r") as metadata_file:
            experiment_metadata = json.load(metadata_file)
    return experiment_metadata


def generate_no_comparison_table(
    results_file, experiment_metadata, most_interesting_metrics, client_metrics, server_metrics
):
    print(experiment_metadata)
    with redirect_stdout(results_file):
        # Generate metadata table
        print("<details>")
        print("<summary>Experiment metadata - Expand here</summary>")
        print("\n")

        print("###### Experiment metadata: ######\n")
        writer = MarkdownTableWriter(
            # table_name="Interesting metrics",
            headers=["Key (this run)", "Value (this run)", "Value (compared run)"],
            value_matrix=[
                [
                    k,
                    experiment_metadata[k],
                    "N/A",
                ]
                for k in experiment_metadata
            ],
            margin=1,  # add a whitespace for both sides of each cell
        )
        writer.write_table()
        print("\n")
        print("</details>")
        print("\n")

        # Generate most interesting metric table
        interesting_metrics = {}
        for k in client_metrics:
            if k in most_interesting_metrics:
                interesting_metrics[k] = client_metrics[k]
        for k in server_metrics:
            if k in most_interesting_metrics:
                interesting_metrics[k] = server_metrics[k]
        if len(interesting_metrics) == 0:
            print("NO INTERESTING METRICS\n")
        else:
            print("###### SUMMARY OF MOST INTERESTING METRICS: ######\n")

            writer = MarkdownTableWriter(
                # table_name="Interesting metrics",
                headers=["Metric", "Entries", "Average ± Standard Deviation", "Min", "Max"],
                value_matrix=[
                    [
                        k,
                        interesting_metrics[k]["entries"],
                        "{:.3f} ± {:.3f}".format(
                            interesting_metrics[k]["avg"], interesting_metrics[k]["std"]
                        )
                        if interesting_metrics[k]["entries"] > 1
                        else interesting_metrics[k]["avg"],
                        interesting_metrics[k]["min"],
                        interesting_metrics[k]["max"],
                    ]
                    for k in interesting_metrics
                ],
                margin=1,  # add a whitespace for both sides of each cell
                max_precision=3,
            )
            writer.write_table()
            print("\n")

        # Generate client metric
        if len(client_metrics) == 0:
            print("NO CLIENT METRICS\n")
        else:
            print("###### CLIENT METRICS: ######\n")

        writer = MarkdownTableWriter(
            # table_name="Client metrics",
            headers=["Metric", "Entries", "Average ± Standard Deviation", "Min", "Max"],
            value_matrix=[
                [
                    k,
                    client_metrics[k]["entries"],
                    "{:.3f} ± {:.3f}".format(client_metrics[k]["avg"], client_metrics[k]["std"])
                    if client_metrics[k]["entries"] > 1
                    else client_metrics[k]["avg"],
                    client_metrics[k]["min"],
                    client_metrics[k]["max"],
                ]
                for k in client_metrics
            ],
            margin=1,  # add a whitespace for both sides of each cell
            max_precision=3,
        )
        writer.write_table()
        print("\n")

        if len(server_metrics) == 0:
            print("NO SERVER METRICS\n")
        else:
            print("###### SERVER METRICS: ######\n")

        writer = MarkdownTableWriter(
            # table_name="Client metrics",
            headers=["Metric", "Entries", "Average ± Standard Deviation", "Min", "Max"],
            value_matrix=[
                [
                    k,
                    server_metrics[k]["entries"],
                    "{:.3f} ± {:.3f}".format(server_metrics[k]["avg"], server_metrics[k]["std"])
                    if server_metrics[k]["entries"] > 1
                    else server_metrics[k]["avg"],
                    server_metrics[k]["min"],
                    server_metrics[k]["max"],
                ]
                for k in server_metrics
            ],
            margin=1,  # add a whitespace for both sides of each cell
            max_precision=3,
        )
        writer.write_table()


def generate_comparison_table(
    results_file,
    most_interesting_metrics,
    experiment_metadata,
    compared_experiment_metadata,
    client_metrics,
    server_metrics,
    client_table_entries,
    server_table_entries,
    branch_name,
):
    with redirect_stdout(results_file):
        # Generate metadata table
        print("<details>")
        print("<summary>Experiment metadata - Expand here</summary>")
        print("\n")

        print("###### Experiment metadata: ######\n")
        writer = MarkdownTableWriter(
            # table_name="Interesting metrics",
            headers=["Key (this run)", "Value (this run)", "Value (compared run)"],
            value_matrix=[
                [
                    k,
                    experiment_metadata[k],
                    "not found"
                    if not compared_experiment_metadata or k not in compared_experiment_metadata
                    else compared_experiment_metadata[k],
                ]
                for k in experiment_metadata
            ],
            margin=1,  # add a whitespace for both sides of each cell
        )
        writer.write_table()
        print("\n")
        print("</details>")
        print("\n")

        # Generate most interesting metric table
        interesting_metrics = []
        for row in client_table_entries:
            if row[0] in most_interesting_metrics:
                interesting_metrics.append(row)
        for row in server_table_entries:
            if row[0] in most_interesting_metrics:
                interesting_metrics.append(row)
        if len(interesting_metrics) == 0:
            print("NO INTERESTING METRICS\n")
        else:
            print("###### SUMMARY OF MOST INTERESTING METRICS: ######\n")

            writer = MarkdownTableWriter(
                # table_name="Interesting metrics",
                headers=[
                    "Metric",
                    "Entries (this branch)",
                    "Average ± Standard Deviation (this branch)",
                    "Average ± Standard Deviation ({})".format(branch_name),
                    "Delta",
                    "",
                ],
                value_matrix=[i for i in interesting_metrics],
                margin=1,  # add a whitespace for both sides of each cell
                max_precision=3,
            )
            writer.write_table()
            print("\n")

        # Generate client table
        if len(client_metrics) == 0:
            print("NO CLIENT METRICS\n")
        else:
            print("###### CLIENT METRICS: ######\n")

        writer = MarkdownTableWriter(
            # table_name="Client metrics",
            headers=[
                "Metric",
                "Entries (this branch)",
                "Average ± Standard Deviation (this branch)",
                "Average ± Standard Deviation ({})".format(branch_name),
                "Delta",
                "",
            ],
            value_matrix=[i for i in client_table_entries],
            margin=1,  # add a whitespace for both sides of each cell
            max_precision=3,
        )
        writer.write_table()
        print("\n")

        if len(server_metrics) == 0:
            print("NO SERVER METRICS\n")
        else:
            print("###### SERVER METRICS: ######\n")

        # Generate server table
        writer = MarkdownTableWriter(
            # table_name="Server metrics",
            headers=[
                "Metric",
                "Entries (this branch)",
                "Average ± Standard Deviation (this branch)",
                "Average ± Standard Deviation ({})".format(branch_name),
                "Delta",
                "",
            ],
            value_matrix=[i for i in server_table_entries],
            margin=1,  # add a whitespace for both sides of each cell
            max_precision=3,
        )
        writer.write_table()


def create_github_gist_post(github_gist_token, title, body):
    # Display the results as a Github Gist
    client = Github(github_gist_token)
    gh_auth_user = client.get_user()
    gist = gh_auth_user.create_gist(
        public=False,
        files={
            "performance_results.md": InputFileContent(body),
        },
        description=title,
    )
    print("Posted performance results to secret gist: {}".format(gist.html_url))
    return gist.html_url


def create_slack_post(slack_webhook, title, gist_url):
    if slack_webhook:
        slack_post(
            slack_webhook,
            body="Daily E2E dev benchmark results: {}\n".format(gist_url),
            slack_username="Whist Bot",
            title=title,
        )


def search_open_PR():
    branch_name = ""

    # In CI, the PR branch name is saved in GITHUB_REF_NAME, or in the GITHUB_HEAD_REF environment variable (in case this script is being run as part of a PR)
    b = os.getenv("GITHUB_REF_NAME").split("/")
    if len(b) != 2 or not b[0].isnumeric() or b[1] != "merge":
        branch_name = os.getenv("GITHUB_REF_NAME")
    else:
        branch_name = os.getenv("GITHUB_HEAD_REF")

    # Search for an open PR connected to the current branch. If found, post results as a comment on that PR's page.
    result = ""
    if len(branch_name) > 0:
        gh_cmd = "gh pr list -H {}".format(branch_name)
        result = subprocess.check_output(gh_cmd, shell=True).decode("utf-8").strip().split()
    pr_number = -1
    if len(result) >= 3 and branch_name in result and result[0].isnumeric():
        pr_number = int(result[0])
    return pr_number


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

if __name__ == "__main__":
    # Grab environmental variables of interest
    if not os.environ.get("GITHUB_REF_NAME"):
        print("GITHUB_REF_NAME is not set! Skipping benchmark notification.")
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

    client_metrics2, server_metrics2 = extract_metrics(client_log_file, server_log_file)
    compared_client_metrics2 = {}
    compared_server_metrics2 = {}

    compared_branch_name = args.compared_branch_name

    # If we are looking to compare the results with the latest run on a branch, we need to download the relevant files first
    if compared_branch_name != "":
        download_latest_logs(compared_branch_name)
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

    experiment_metadata = parse_metadata(logs_root_dir)

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

    title = "Protocol End-to-End Streaming Test Results - {} UTC".format(test_time)
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
