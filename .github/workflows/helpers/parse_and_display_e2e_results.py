import os
import sys
import time
import boto3
import numpy as np
from pytablewriter import MarkdownTableWriter
from contextlib import redirect_stdout

# Get params
if not os.environ.get("GITHUB_REF_NAME"):
    print("No GitHub branch/PR number! Skipping benchmark notification.")
    sys.exit(-1)
if not os.environ.get("GITHUB_GIST_TOKEN") or not os.environ.get("GITHUB_TOKEN"):
    print("No GitHub tokens available to post the results to Gist/Github PR body!")
    sys.exit(-1)

github_ref_name = os.environ["GITHUB_REF_NAME"]
github_gist_token = os.environ["GITHUB_GIST_TOKEN"]
github_token = os.environ["GITHUB_TOKEN"]

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


# Here, we parse the test results into a .info file, which can be read and displayed on the GitHub PR
# Create output .info file
results_file = open("streaming_e2e_test_results.info", "w+")

# Find the path to the folder with the most recent E2E protocol logs
logs_root_dir = "perf_logs"
test_time = ""
for folder_name in os.listdir("./perf_logs"):
    if time.strftime("%Y_%m_%d@") in folder_name:
        logs_root_dir = os.path.join(logs_root_dir, folder_name)
        test_time = folder_name
        break
if logs_root_dir == "perf_logs":
    print("Error: protocol logs not found!")
    exit(-1)


# Check if the log files with metrics are present
client_log_file = os.path.join(logs_root_dir, "client", "client.log")
server_log_file = os.path.join(logs_root_dir, "server", "server.log")

if not os.path.isfile(client_log_file):
    print("Error, client log file {} does not exist".format(client_log_file))
    exit(-1)
if not os.path.isfile(server_log_file):
    print("Error, server log file {} does not exist".format(server_log_file))
    exit(-1)

# Extract the metric values and save them in two dictionaries
client_metrics = {}
server_metrics = {}

with open(client_log_file, "r") as f:
    for line in f.readlines():
        if line[0] == "{":
            l = line.strip().split()
            metric_name = l[1].strip('"')
            metric_value = float(l[3].strip('"'))
            if metric_name not in client_metrics:
                client_metrics[metric_name] = [metric_value]
            else:
                client_metrics[metric_name].append(metric_value)

with open(server_log_file, "r") as f:
    for line in f.readlines():
        if line[0] == "{":
            l = line.strip().split()
            metric_name = l[1].strip('"')
            metric_value = float(l[3].strip('"'))
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


dev_client_metrics2 = {}
dev_server_metrics2 = {}

# If we are not on dev, we need to compare the results with the latest dev run, so we need to download the relevant files
if github_ref_name != "dev":

    client = boto3.client("s3")
    result = client.list_objects(
        Bucket="whist-e2e-protocol-test-logs", Prefix="dev/", Delimiter="/"
    )
    subfolder_name = result.get("CommonPrefixes")[-1].get("Prefix").split("/")[-2]

    s3_resource = boto3.resource("s3")
    bucket = s3_resource.Bucket("whist-e2e-protocol-test-logs")

    if os.path.exists("dev"):
        os.system("rm -rf dev")
    os.mkdir("dev")

    dev_client_log_path = os.path.join(".", "dev", "client.log")
    dev_server_log_path = os.path.join(".", "dev", "server.log")

    for obj in bucket.objects.filter(Prefix="{}/{}".format("dev", subfolder_name)):
        if "client.log" in obj.key:
            bucket.download_file(obj.key, dev_client_log_path)
        elif "server.log" in obj.key:
            bucket.download_file(obj.key, dev_server_log_path)

    if not os.path.isfile(dev_client_log_path) or not os.path.isfile(dev_server_log_path):
        print(
            "Could not get dev client/server logs. Unable to compare performance results to latest dev measurements."
        )
    else:
        # Extract the metric values and save them in a dictionary
        dev_client_metrics = {}
        dev_server_metrics = {}

        with open(dev_client_log_path, "r") as f:
            for line in f.readlines():
                if line[0] == "{":
                    l = line.strip().split()
                    metric_name = l[1].strip('"')
                    metric_value = float(l[3].strip('"'))
                    if metric_name not in dev_client_metrics:
                        dev_client_metrics[metric_name] = [metric_value]
                    else:
                        dev_client_metrics[metric_name].append(metric_value)

        with open(dev_server_log_path, "r") as f:
            for line in f.readlines():
                if line[0] == "{":
                    l = line.strip().split()
                    metric_name = l[1].strip('"')
                    metric_value = float(l[3].strip('"'))
                    if metric_name not in dev_server_metrics:
                        dev_server_metrics[metric_name] = [metric_value]
                    else:
                        dev_server_metrics[metric_name].append(metric_value)

        for k in dev_client_metrics:
            dev_client_metrics[k] = np.array(dev_client_metrics[k])
            dev_client_metrics2[k] = {
                "entries": len(dev_client_metrics[k]),
                "avg": np.mean(dev_client_metrics[k]),
                "std": np.std(dev_client_metrics[k]),
                "max": np.max(dev_client_metrics[k]),
                "min": np.min(dev_client_metrics[k]),
            }

        for k in dev_server_metrics:
            dev_server_metrics[k] = np.array(dev_server_metrics[k])
            dev_server_metrics2[k] = {
                "entries": len(dev_server_metrics[k]),
                "avg": np.mean(dev_server_metrics[k]),
                "std": np.std(dev_server_metrics[k]),
                "max": np.max(dev_server_metrics[k]),
                "min": np.min(dev_server_metrics[k]),
            }


# Generate the report
with redirect_stdout(results_file):
    if os.environ.get("GITHUB_REF_NAME") == "dev":

        # Generate most interesting metric table
        interesting_metrics = {}
        for k in client_metrics2:
            if k in most_interesting_metrics:
                interesting_metrics[k] = client_metrics2[k]
        for k in server_metrics2:
            if k in most_interesting_metrics:
                interesting_metrics[k] = server_metrics2[k]
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
            writer.write_tab()
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
                    client_metrics2[k]["entries"],
                    "{:.3f} ± {:.3f}".format(client_metrics2[k]["avg"], client_metrics2[k]["std"])
                    if client_metrics2[k]["entries"] > 1
                    else client_metrics2[k]["avg"],
                    client_metrics2[k]["min"],
                    client_metrics2[k]["max"],
                ]
                for k in client_metrics2
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
                    server_metrics2[k]["entries"],
                    "{:.3f} ± {:.3f}".format(server_metrics2[k]["avg"], server_metrics2[k]["std"])
                    if server_metrics2[k]["entries"] > 1
                    else server_metrics2[k]["avg"],
                    server_metrics2[k]["min"],
                    server_metrics2[k]["max"],
                ]
                for k in server_metrics2
            ],
            margin=1,  # add a whitespace for both sides of each cell
            max_precision=3,
        )
        writer.write_table()
    else:
        # Augment dictionaries with deltas wrt to dev, if available
        for k in client_metrics2:
            if k in dev_client_metrics2:
                client_metrics2[k]["dev_entries"] = dev_client_metrics2[k]["entries"]
                client_metrics2[k]["dev_avg"] = round(dev_client_metrics2[k]["avg"], 3)
                client_metrics2[k]["dev_std"] = round(dev_client_metrics2[k]["std"], 3)
                client_metrics2[k]["delta"] = round(
                    client_metrics2[k]["avg"] - client_metrics2[k]["dev_avg"], 3
                )
                if client_metrics2[k]["delta"] == 0:
                    client_metrics2[k]["delta"] = "-"
                    client_metrics2[k]["delta_pctg"] = "-"
                elif client_metrics2[k]["dev_avg"] == 0:
                    client_metrics2[k]["delta_pctg"] = "nan"
                else:
                    client_metrics2[k]["delta_pctg"] = round(
                        (client_metrics2[k]["delta"] / client_metrics2[k]["dev_avg"]), 3
                    )
            else:
                client_metrics2[k]["dev_avg"] = "N/A"
                client_metrics2[k]["dev_std"] = "N/A"
                client_metrics2[k]["delta"] = "N/A"
                client_metrics2[k]["delta_pctg"] = "N/A"

        # Create client table entries with the desired format
        client_table_entries = []
        for k in client_metrics2:
            new_entry = [k, client_metrics2[k]["entries"]]

            avg_stdv_this_branch = ""
            if client_metrics2[k]["entries"] > 1:
                avg_stdv_this_branch = "{:.3f} ± {:.3f}".format(
                    client_metrics2[k]["avg"], client_metrics2[k]["std"]
                )
            else:
                avg_stdv_this_branch = "{:.3f}".format(client_metrics2[k]["avg"])

            new_entry.append(avg_stdv_this_branch)

            avg_stdv_dev = ""
            if client_metrics2[k]["dev_avg"] == "N/A" or client_metrics2[k]["dev_std"] == "N/A":
                avg_stdv_dev = "N/A"
            elif client_metrics2[k]["dev_entries"] > 1:
                avg_stdv_dev = "{:.3f} ± {:.3f}".format(
                    client_metrics2[k]["dev_avg"], client_metrics2[k]["dev_std"]
                )
            else:
                avg_stdv_dev = "{:.3f}".format(client_metrics2[k]["dev_avg"])

            new_entry.append(avg_stdv_dev)

            delta_formatted = client_metrics2[k]["delta"]
            if client_metrics2[k]["delta"] != "-" and client_metrics2[k]["delta"] != "N/A":
                delta_formatted = "{:.3f}".format(delta_formatted)
                delta_pctg_formatted = client_metrics2[k]["delta_pctg"]
                if (
                    client_metrics2[k]["delta_pctg"] != "-"
                    and client_metrics2[k]["delta_pctg"] != "nan"
                    and client_metrics2[k]["delta_pctg"] != "N/A"
                ):
                    delta_pctg_formatted = "{:.3f}".format(delta_pctg_formatted * 100.0)
                new_entry.append("{} ({}%)".format(delta_formatted, delta_pctg_formatted))
            else:
                new_entry.append(delta_formatted)

            emoji_delta = ""
            if (
                client_metrics2[k]["delta"] != "-"
                and client_metrics2[k]["delta"] != "N/A"
                and client_metrics2[k]["delta_pctg"] != "-"
                and client_metrics2[k]["delta_pctg"] != "nan"
                and client_metrics2[k]["delta_pctg"] != "N/A"
            ):
                if client_metrics2[k]["delta"] > 0:
                    emoji_delta = "⬆️"
                else:
                    emoji_delta = "⬇️"

            new_entry.append(emoji_delta)
            client_table_entries.append(new_entry)

        for k in server_metrics2:
            if k in dev_server_metrics2:
                server_metrics2[k]["dev_entries"] = dev_server_metrics2[k]["entries"]
                server_metrics2[k]["dev_avg"] = round(dev_server_metrics2[k]["avg"], 3)
                server_metrics2[k]["dev_std"] = round(dev_server_metrics2[k]["std"], 3)
                server_metrics2[k]["delta"] = round(
                    server_metrics2[k]["avg"] - server_metrics2[k]["dev_avg"], 3
                )
                if server_metrics2[k]["delta"] == 0:
                    server_metrics2[k]["delta"] = "-"
                    server_metrics2[k]["delta_pctg"] = "-"
                elif server_metrics2[k]["dev_avg"] == 0:
                    server_metrics2[k]["delta_pctg"] = "nan"
                else:
                    server_metrics2[k]["delta_pctg"] = round(
                        (server_metrics2[k]["delta"] / server_metrics2[k]["dev_avg"]), 3
                    )
            else:
                server_metrics2[k]["dev_avg"] = "N/A"
                server_metrics2[k]["dev_std"] = "N/A"
                server_metrics2[k]["delta"] = "N/A"
                server_metrics2[k]["delta_pctg"] = "N/A"

        # Create server table entries with the desired format
        server_table_entries = []
        for k in server_metrics2:
            new_entry = [k, server_metrics2[k]["entries"]]

            avg_stdv_this_branch = ""
            if server_metrics2[k]["entries"] > 1:
                avg_stdv_this_branch = "{:.3f} ± {:.3f}".format(
                    server_metrics2[k]["avg"], server_metrics2[k]["std"]
                )
            else:
                avg_stdv_this_branch = "{:.3f}".format(server_metrics2[k]["avg"])

            new_entry.append(avg_stdv_this_branch)

            avg_stdv_dev = ""
            if server_metrics2[k]["dev_avg"] == "N/A" or server_metrics2[k]["dev_std"] == "N/A":
                avg_stdv_dev = "N/A"
            elif server_metrics2[k]["dev_entries"] > 1:
                avg_stdv_dev = "{:.3f} ± {:.3f}".format(
                    server_metrics2[k]["dev_avg"], server_metrics2[k]["dev_std"]
                )
            else:
                avg_stdv_dev = "{:.3f}".format(server_metrics2[k]["dev_avg"])

            new_entry.append(avg_stdv_dev)

            delta_formatted = server_metrics2[k]["delta"]
            if server_metrics2[k]["delta"] != "-" and server_metrics2[k]["delta"] != "N/A":
                delta_formatted = "{:.3f}".format(delta_formatted)
                delta_pctg_formatted = server_metrics2[k]["delta_pctg"]
                if (
                    server_metrics2[k]["delta_pctg"] != "-"
                    and server_metrics2[k]["delta_pctg"] != "nan"
                    and server_metrics2[k]["delta_pctg"] != "N/A"
                ):
                    delta_pctg_formatted = "{:.3f}".format(delta_pctg_formatted * 100.0)
                new_entry.append("{} ({}%)".format(delta_formatted, delta_pctg_formatted))
            else:
                new_entry.append(delta_formatted)

            emoji_delta = ""
            if (
                server_metrics2[k]["delta"] != "-"
                and server_metrics2[k]["delta"] != "N/A"
                and server_metrics2[k]["delta_pctg"] != "-"
                and server_metrics2[k]["delta_pctg"] != "nan"
                and server_metrics2[k]["delta_pctg"] != "N/A"
            ):
                if server_metrics2[k]["delta"] > 0:
                    emoji_delta = "⬆️"
                else:
                    emoji_delta = "⬇️"

            new_entry.append(emoji_delta)
            server_table_entries.append(new_entry)

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
                    "Average ± Standard Deviation (dev)",
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
                "Average ± Standard Deviation (dev)",
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
                "Average ± Standard Deviation (dev)",
                "Delta",
                "",
            ],
            value_matrix=[i for i in server_table_entries],
            margin=1,  # add a whitespace for both sides of each cell
            max_precision=3,
        )
        writer.write_table()

results_file.close()

#######################################################################################

title = "Protocol End-to-End Streaming Test Results - {}".format(test_time)
github_repo = "whisthq/whist"
# Adding timestamp to prevent overwrite of message
identifier = "AUTOMATED_STREAMING_E2E_TEST_RESULTS_MESSAGE - {}".format(test_time)
f = open("streaming_e2e_test_results.info", "r")
body = f.read()
f.close()


# Display the results as a Github Gist
from github import Github, InputFileContent

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


sys.path.append(".github/workflows/helpers")

# Post updates to Slack channel if we are on dev
if github_ref_name == "dev":
    from notifications.slack_bot import slack_post

    if not os.environ.get("SLACK_WEBHOOK"):
        print("Error, cannot post updates to Slack because SLACK_WEBHOOK was not set")
    else:
        slack_webhook = os.environ.get("SLACK_WEBHOOK")
        slack_post(
            slack_webhook,
            body="Daily E2E dev benchmark results: {}\n".format(gist.html_url),
            slack_username="Whist Bot",
            title=title,
        )
else:
    from notifications.github_bot import github_comment_update

    github_issue = int(os.environ["GITHUB_REF_NAME"].split("/")[0])

    github_comment_update(
        github_token,
        github_repo,
        github_issue,
        identifier,
        body,
        title=title,
    )
