#!/usr/bin/env python3

import os
import sys
import time
import numpy as np
import math
from datetime import datetime, timedelta

# add the current directory to the path no matter where this is called from
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "."))


def extract_metrics(client_log_file, server_log_file):
    """
    Extract all the available server-side and client-side performance metrics for a
    given run and save the values two dictionaries. Each dictionary key corresponds
    to a metric, and each value is itself a dictionary, containing the number of
    entries for that metric and the mean (or max/min).

    Args:
        client_log_file (str): The path to the file (usually client.log) containing
                                the client-side logs with the metrics
        server_log_file (str): The path to the file (usually server.log) containing
                                the server-side logs with the metrics
    Returns:
        client_metrics2 (dict): the dictionary containing all the client metrics.
        server_metrics2 (dict): the dictionary containing all the server metrics.
    """
    client_metrics = {}
    server_metrics = {}

    with open(client_log_file, "r") as f:
        for line in f.readlines():
            if "METRIC" in line:
                l = line.strip().split()
                metric_name = l[-3].strip('"')
                metric_values = l[-1].strip('"').split(",")

                count = 0
                normal_sum = 0.0
                min_val = np.inf
                max_val = -np.inf

                if len(metric_values) == 1:
                    count = 1
                    if metric_name[0:4] == "MAX_":
                        max_val = float(metric_values[0])
                    elif metric_name[0:4] == "MIN_":
                        min_val = float(metric_values[0])
                    else:
                        normal_sum = float(metric_values[0])
                else:
                    count = int(metric_values[0])
                    normal_sum = float(metric_values[1])

                assert count >= 0
                assert normal_sum >= 0

                if metric_name not in client_metrics:
                    client_metrics[metric_name] = {
                        "entries": 0,
                        "sum": 0,
                        "min_val": min_val,
                        "max_val": max_val,
                    }

                client_metrics[metric_name]["entries"] += count
                client_metrics[metric_name]["min_val"] = min(
                    client_metrics[metric_name]["min_val"], min_val
                )
                client_metrics[metric_name]["max_val"] = max(
                    client_metrics[metric_name]["max_val"], max_val
                )
                client_metrics[metric_name]["sum"] += normal_sum

    with open(server_log_file, "r") as f:
        for line in f.readlines():
            if "METRIC" in line:
                l = line.strip().split()
                metric_name = l[-3].strip('"')
                # metric_value = float(l[-1].strip('"'))
                metric_values = l[-1].strip('"').split(",")

                count = 0
                normal_sum = 0.0
                min_val = np.inf
                max_val = -np.inf

                if len(metric_values) == 1:
                    count = 1
                    if metric_name[0:4] == "MAX_":
                        max_val = float(metric_values[0])
                    elif metric_name[0:4] == "MIN_":
                        min_val = float(metric_values[0])
                    else:
                        normal_sum = float(metric_values[0])
                else:
                    count = int(metric_values[0])
                    normal_sum = float(metric_values[1])

                assert count >= 0
                assert normal_sum >= 0

                if metric_name not in server_metrics:
                    server_metrics[metric_name] = {
                        "entries": 0,
                        "min_val": min_val,
                        "max_val": max_val,
                        "sum": 0,
                    }

                server_metrics[metric_name]["entries"] += count
                server_metrics[metric_name]["min_val"] = min(
                    server_metrics[metric_name]["min_val"], min_val
                )
                server_metrics[metric_name]["max_val"] = max(
                    server_metrics[metric_name]["max_val"], max_val
                )
                server_metrics[metric_name]["sum"] += normal_sum

    client_metrics2 = {}
    server_metrics2 = {}

    for metric_name in client_metrics:
        if client_metrics[metric_name]["min_val"] < np.inf:
            client_metrics2[metric_name] = {
                "entries": client_metrics[metric_name]["entries"],
                "avg": client_metrics[metric_name]["min_val"],
            }
        elif client_metrics[metric_name]["max_val"] > -np.inf:
            client_metrics2[metric_name] = {
                "entries": client_metrics[metric_name]["entries"],
                "avg": client_metrics[metric_name]["max_val"],
            }
        else:
            assert client_metrics[metric_name]["sum_of_squares"] >= 0.0
            client_metrics2[metric_name] = {
                "entries": client_metrics[metric_name]["entries"],
                "avg": client_metrics[metric_name]["sum"] / client_metrics[metric_name]["entries"],
            }

    for metric_name in server_metrics:
        if server_metrics[metric_name]["min_val"] < np.inf:
            server_metrics2[metric_name] = {
                "entries": server_metrics[metric_name]["entries"],
                "avg": server_metrics[metric_name]["min_val"],
            }
        elif server_metrics[metric_name]["max_val"] > -np.inf:
            server_metrics2[metric_name] = {
                "entries": server_metrics[metric_name]["entries"],
                "avg": server_metrics[metric_name]["max_val"],
            }
        else:
            server_metrics2[metric_name] = {
                "entries": server_metrics[metric_name]["entries"],
                "avg": server_metrics[metric_name]["sum"] / server_metrics[metric_name]["entries"],
            }

    return client_metrics2, server_metrics2


def compute_deltas(
    client_dictionary, server_dictionary, compared_client_dictionary, compared_server_dictionary
):
    """
    Give the metric values for two runs, augment the client/server dictionaries of the current
    run with the values from the compared run. Also compute difference (delta) and percentage
    of change (delta pctg) for each metric. Handle corner cases such as when a key/value is
    missing on the current or the compared run, when the difference is 0, or when the percentage
    change is 0. Round all results to the third decimal place. Finally, create the entries
    for the markdown comparison table

    Args:
        client_dictionary (dict): The dictionary containing the metrics key-value pairs for the
                                    client from the current run
        server_dictionary (dict): The dictionary containing the metrics key-value pairs for the
                                    server from the current run
        compared_client_dictionary (dict): The dictionary containing the metrics key-value pairs
                                            for the client from the compared run
        compared_server_dictionary (dict): The dictionary containing the metrics key-value pairs
                                            for the server from the compared run
    Returns:
        client_table_entries (list): the list containing the entries for the client markdown
                                    comparison table. Each element in the list of lists corresponds
                                    to one row.
        server_table_entries (list): the list containing the entries for the server markdown
                                    comparison table. Each element in the list of lists corresponds
                                    to one row.
    """

    # Augment client dictionary with metrics from client in compared run
    for k in client_dictionary:
        if k in compared_client_dictionary:
            client_dictionary[k]["compared_entries"] = compared_client_dictionary[k]["entries"]
            client_dictionary[k]["compared_avg"] = round(compared_client_dictionary[k]["avg"], 3)
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
            client_dictionary[k]["delta"] = "N/A"
            client_dictionary[k]["delta_pctg"] = "N/A"

    # Create client table entries with the desired format
    client_table_entries = []
    for k in client_dictionary:
        new_entry = [
            k,
            client_dictionary[k]["entries"],
            "{:.3f}".format(client_dictionary[k]["avg"]),
        ]

        if client_dictionary[k]["compared_avg"] == "N/A":
            new_entry.append("N/A")
        else:
            new_entry.append("{:.3f}".format(client_dictionary[k]["compared_avg"]))

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

    # Augment server dictionary with metrics from server in compared run
    for k in server_dictionary:
        if k in compared_server_dictionary:
            server_dictionary[k]["compared_entries"] = compared_server_dictionary[k]["entries"]
            server_dictionary[k]["compared_avg"] = round(compared_server_dictionary[k]["avg"], 3)
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
            server_dictionary[k]["delta"] = "N/A"
            server_dictionary[k]["delta_pctg"] = "N/A"

    # Create server table entries with the desired format
    server_table_entries = []
    for k in server_dictionary:
        new_entry = [
            k,
            server_dictionary[k]["entries"],
            "{:.3f}".format(server_dictionary[k]["avg"]),
        ]

        if server_dictionary[k]["compared_avg"] == "N/A":
            new_entry.append("N/A")
        else:
            new_entry.append("{:.3f}".format(server_dictionary[k]["compared_avg"]))

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
