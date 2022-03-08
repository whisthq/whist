#!/usr/bin/env python3

import os
import sys
import time
import numpy as np
import math
from datetime import datetime, timedelta

# add the current directory to the path no matter where this is called from
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "."))


def parse_line(line):
    def is_number(element):
        try:
            float(element)
            return True
        except ValueError:
            return False

    l = line.strip().split()
    metric_name = list(filter(lambda x: '"' in x, l))[0].strip('"')
    metric_values = list(filter(lambda x: is_number(x), l))

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
        count = int(float(metric_values[1]))
        normal_sum = float(metric_values[2])

    assert count >= 0
    assert normal_sum >= 0

    return {
        metric_name: metric_name,
        max_val: max_val,
        min_val: min_val,
        count: count,
        normal_sum: normal_sum,
    }


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
        experiment_metrics (list): A list containing two dictionaries with the client
                                 and server metrics, respectively.
    """

    experiment_metrics = []

    for log_file in (client_log_file, server_log_file):
        new_experiment_metrics = {}
        with open(log_file, "r") as f:
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

                    if metric_name not in new_experiment_metrics:
                        new_experiment_metrics[metric_name] = {
                            "entries": 0,
                            "sum": 0,
                            "min_val": min_val,
                            "max_val": max_val,
                        }

                    new_experiment_metrics[metric_name]["entries"] += count
                    new_experiment_metrics[metric_name]["min_val"] = min(
                        new_experiment_metrics[metric_name]["min_val"], min_val
                    )
                    new_experiment_metrics[metric_name]["max_val"] = max(
                        new_experiment_metrics[metric_name]["max_val"], max_val
                    )
                    new_experiment_metrics[metric_name]["sum"] += normal_sum
        for metric_name in new_experiment_metrics:
            if new_experiment_metrics[metric_name]["min_val"] < np.inf:
                new_experiment_metrics[metric_name]["avg"] = new_experiment_metrics[metric_name][
                    "min_val"
                ]
            elif new_experiment_metrics[metric_name]["max_val"] > -np.inf:
                new_experiment_metrics[metric_name]["avg"] = new_experiment_metrics[metric_name][
                    "max_val"
                ]
            else:
                new_experiment_metrics[metric_name]["avg"] = (
                    new_experiment_metrics[metric_name]["sum"]
                    / new_experiment_metrics[metric_name]["entries"]
                )

        experiment_metrics.append(new_experiment_metrics)

    return experiment_metrics


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
        table_entries (list): A list with two sublists. The two sublists contain the entries for
                                the markdown comparison tables for the client and server,
                                respectively. Each item in the sublists corresponds to one row
                                in the table.
    """
    metrics_dictionaries = [client_dictionary, server_dictionary]
    compared_dictionaries = [compared_client_dictionary, compared_server_dictionary]
    table_entries = []

    for dictionary, compared_dictionary in zip(metrics_dictionaries, compared_dictionaries):
        # Augment experiment dictionary with metrics from experiment we are comparing to
        for k in dictionary:
            if k in compared_dictionary:
                dictionary[k]["compared_entries"] = compared_dictionary[k]["entries"]
                dictionary[k]["compared_avg"] = round(compared_dictionary[k]["avg"], 3)
                dictionary[k]["delta"] = round(
                    dictionary[k]["avg"] - dictionary[k]["compared_avg"], 3
                )
                if dictionary[k]["delta"] == 0:
                    dictionary[k]["delta"] = "-"
                    dictionary[k]["delta_pctg"] = "-"
                elif dictionary[k]["compared_avg"] == 0:
                    dictionary[k]["delta_pctg"] = "nan"
                else:
                    dictionary[k]["delta_pctg"] = round(
                        (dictionary[k]["delta"] / dictionary[k]["compared_avg"]), 3
                    )
            else:
                dictionary[k]["compared_avg"] = "N/A"
                dictionary[k]["delta"] = "N/A"
                dictionary[k]["delta_pctg"] = "N/A"

        # Create table entries with the desired format
        new_table_entries = []
        for k in dictionary:
            new_entry = [
                k,
                dictionary[k]["entries"],
                f"{dictionary[k]['avg']:.3f}",
            ]

            if dictionary[k]["compared_avg"] == "N/A":
                new_entry.append("N/A")
            else:
                new_entry.append(f"{dictionary[k]['compared_avg']:.3f}")

            delta_formatted = dictionary[k]["delta"]
            if dictionary[k]["delta"] != "-" and dictionary[k]["delta"] != "N/A":
                delta_formatted = f"{delta_formatted:.3f}"
                delta_pctg_formatted = dictionary[k]["delta_pctg"]
                if (
                    dictionary[k]["delta_pctg"] != "-"
                    and dictionary[k]["delta_pctg"] != "nan"
                    and dictionary[k]["delta_pctg"] != "N/A"
                ):
                    delta_pctg_formatted = f"{delta_pctg_formatted * 100.0:.3f}"
                new_entry.append(f"{delta_formatted} ({delta_pctg_formatted}%)")
            else:
                new_entry.append(delta_formatted)

            emoji_delta = ""
            if (
                dictionary[k]["delta"] != "-"
                and dictionary[k]["delta"] != "N/A"
                and dictionary[k]["delta_pctg"] != "-"
                and dictionary[k]["delta_pctg"] != "nan"
                and dictionary[k]["delta_pctg"] != "N/A"
            ):
                if dictionary[k]["delta"] > 0:
                    emoji_delta = "⬆️"
                else:
                    emoji_delta = "⬇️"

            new_entry.append(emoji_delta)
            new_table_entries.append(new_entry)
        table_entries.append(new_table_entries)

    return table_entries
