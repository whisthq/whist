#!/usr/bin/env python3

import os
import sys
import time
import numpy as np
from datetime import datetime, timedelta

# add the current directory to the path no matter where this is called from
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "."))


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
