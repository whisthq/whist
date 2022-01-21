import os, sys, time, boto3
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
                    "{:.4f} ± {:.4f}".format(client_metrics2[k]["avg"], client_metrics2[k]["std"])
                    if client_metrics2[k]["entries"] > 1
                    else client_metrics2[k]["avg"],
                    client_metrics2[k]["min"],
                    client_metrics2[k]["max"],
                ]
                for k in client_metrics2
            ],
            margin=1,  # add a whitespace for both sides of each cell
            max_precision=4,
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
                    "{:.4f} ± {:.4f}".format(server_metrics2[k]["avg"], server_metrics2[k]["std"])
                    if server_metrics2[k]["entries"] > 1
                    else server_metrics2[k]["avg"],
                    server_metrics2[k]["min"],
                    server_metrics2[k]["max"],
                ]
                for k in server_metrics2
            ],
            margin=1,  # add a whitespace for both sides of each cell
            max_precision=4,
        )
        writer.write_table()
    else:
        # Augment dictionaries with deltas wrt to dev, if available
        for k in client_metrics2:
            if k in dev_client_metrics2:
                client_metrics2[k]["dev_avg"] = dev_client_metrics2[k]["avg"]
                client_metrics2[k]["dev_std"] = dev_client_metrics2[k]["std"]
                client_metrics2[k]["delta"] = (
                    client_metrics2[k]["avg"] - client_metrics2[k]["dev_avg"]
                )
                client_metrics2[k]["delta_pctg"] = (
                    (client_metrics2[k]["delta"] / client_metrics2[k]["dev_avg"])
                    if client_metrics2[k]["delta"] >= 0.0001 and client_metrics2[k]["dev_avg"] != 0
                    else -1000
                )
        for k in server_metrics2:
            if k in dev_server_metrics2:
                server_metrics2[k]["dev_avg"] = dev_server_metrics2[k]["avg"]
                server_metrics2[k]["dev_std"] = dev_server_metrics2[k]["std"]
                server_metrics2[k]["delta"] = (
                    server_metrics2[k]["avg"] - server_metrics2[k]["dev_avg"]
                )
                server_metrics2[k]["delta_pctg"] = (
                    (server_metrics2[k]["delta"] / server_metrics2[k]["dev_avg"])
                    if server_metrics2[k]["delta"] >= 0.0001 and server_metrics2[k]["dev_avg"] != 0
                    else -1000
                )

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
            value_matrix=[
                [
                    k,
                    client_metrics2[k]["entries"],
                    "{:.4f} ± {:.4f}".format(client_metrics2[k]["avg"], client_metrics2[k]["std"])
                    if client_metrics2[k]["entries"] > 1
                    else client_metrics2[k]["avg"],
                    "{:.4f} ± {:.4f}".format(
                        client_metrics2[k]["dev_avg"], client_metrics2[k]["dev_std"]
                    )
                    if (
                        "dev_avg" in client_metrics2[k]
                        and "dev_std" in client_metrics2[k]
                        and client_metrics2[k]["entries"] > 1
                    )
                    else client_metrics2[k]["avg"]
                    if ("dev_avg" in client_metrics2[k] and "dev_std" in client_metrics2[k])
                    else "N/A",
                    "{} ({}%)".format(
                        client_metrics2[k]["delta"], client_metrics2[k]["delta_pctg"] * 100.0
                    )
                    if (
                        "delta" in client_metrics2[k]
                        and "delta_pctg" in client_metrics2[k]
                        and client_metrics2[k]["delta"] >= 0.0001
                    )
                    else "-"
                    if ("delta" in client_metrics2[k] and "delta_pctg" in client_metrics2[k])
                    else "N/A",
                    ""
                    if (
                        "delta_pctg" not in client_metrics2[k]
                        or abs(client_metrics2[k]["delta_pctg"]) < 0.01
                        or abs(client_metrics2[k]["delta_pctg"]) > 100
                    )
                    else ("⬆️" if client_metrics2[k]["delta_pctg"] > 0.0 else "⬇️"),
                ]
                for k in client_metrics2
            ],
            margin=1,  # add a whitespace for both sides of each cell
            max_precision=4,
        )
        writer.write_table()
        print("\n")

        if len(server_metrics) == 0:
            print("NO SERVER METRICS\n")
        else:
            print("###### SERVER METRICS: ######\n")

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
            value_matrix=[
                [
                    k,
                    server_metrics2[k]["entries"],
                    "{:.4f} ± {:.4f}".format(server_metrics2[k]["avg"], server_metrics2[k]["std"])
                    if server_metrics2[k]["entries"] > 1
                    else server_metrics2[k]["avg"],
                    "{:.4f} ± {:.4f}".format(
                        server_metrics2[k]["dev_avg"], server_metrics2[k]["dev_std"]
                    )
                    if (
                        "dev_avg" in server_metrics2[k]
                        and "dev_std" in server_metrics2[k]
                        and server_metrics2[k]["entries"] > 1
                    )
                    else server_metrics2[k]["avg"]
                    if ("dev_avg" in server_metrics2[k] and "dev_std" in server_metrics2[k])
                    else "N/A",
                    "{} ({}%)".format(
                        server_metrics2[k]["delta"], server_metrics2[k]["delta_pctg"] * 100.0
                    )
                    if (
                        "delta" in server_metrics2[k]
                        and "delta_pctg" in server_metrics2[k]
                        and server_metrics2[k]["delta"] >= 0.0001
                    )
                    else "-"
                    if ("delta" in server_metrics2[k] and "delta_pctg" in server_metrics2[k])
                    else "N/A",
                    ""
                    if (
                        "delta_pctg" not in server_metrics2[k]
                        or abs(server_metrics2[k]["delta_pctg"]) < 0.01
                        or abs(server_metrics2[k]["delta_pctg"]) > 100
                    )
                    else ("⬆️" if server_metrics2[k]["delta_pctg"] > 0.0 else "⬇️"),
                ]
                for k in server_metrics2
            ],
            margin=1,  # add a whitespace for both sides of each cell
            max_precision=4,
        )
        writer.write_table()

results_file.close()

#######################################################################################

title = "Protocol End-to-End Streaming Test Results - {}".format(test_time)
github_repo = "whisthq/whist"
identifier = "AUTOMATED_STREAMING_E2E_TEST_RESULTS_MESSAGE"
f = open("streaming_e2e_test_results.info", "r")
body = f.read()
f.close()


# Display the results as a Github Gist
from github import Github, InputFileContent

client = Github(github_gist_token)
gh_auth_user = client.get_user()
gist = gh_auth_user.create_gist(
    public=False, files={"performance_results.md": InputFileContent(body)}, description=title
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
