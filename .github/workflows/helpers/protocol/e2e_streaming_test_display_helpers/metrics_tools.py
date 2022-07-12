#!/usr/bin/env python3

import json
import os
import subprocess
import sys
from matplotlib import pyplot as plt
import numpy as np

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
        experiment_metrics (list): A list containing two dictionaries with the client
                            and server metrics, respectively.
    """

    experiment_metrics = []

    for log_file in (client_log_file, server_log_file):
        new_experiment_metrics = {}
        with open(log_file, "r") as f:
            for line in f.readlines():
                if line.startswith("{"):
                    count = 0
                    normal_sum = 0.0
                    min_val = np.inf
                    max_val = -np.inf
                    metric_name = ""

                    l = line.replace("{", "").replace("}", "").strip().split()

                    # For regular metrics, l has the following format:
                    # [..., "<metric_name>", ":", "<local_average>,", "COUNT:", "<count>"]

                    # For MAX/MIN aggregates, l has the following format:
                    # [..., "<MAX or MIN>_<metric_name>", ":", "<max or min value>"]

                    # Finally, some metrics just print a single value:
                    # [..., "<metric_name>", ":", "<single_value>"]

                    # We can use the second-to-last token to distinguish between the two. A regular metric log will print 'COUNT:', a MAX/MIN log will print ':'
                    format_indicator = l[-2].replace('"', "")

                    if format_indicator == "COUNT:":
                        # Regular metric
                        metric_name = l[-5].strip('"')
                        count = int(l[-1])
                        # Remove trailing comma
                        local_average = float(l[-3].strip(","))
                        normal_sum += local_average * count
                    elif format_indicator == ":":
                        # MAX/MIN aggregate
                        count = 1
                        metric_name = l[-3].strip('"')
                        metric_value = float(l[-1])
                        if metric_name[0:4] == "MAX_":
                            max_val = float(metric_value)
                        elif metric_name[0:4] == "MIN_":
                            min_val = float(metric_value)
                        else:
                            # Single-value metric (e.g `HANDSHAKE_CONNECT_TO_SERVER_TIME`) or
                            # Sum metrics (e.g VIDEO_NUM_RECOVERY_FRAMES)
                            metric_name = l[-3].strip('"')
                            metric_value = float(l[-1])
                            count = 1
                            normal_sum += metric_value

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


def generate_comparison_entries(
    client_dictionary,
    server_dictionary,
    compared_client_dictionary,
    compared_server_dictionary,
    most_interesting_metrics,
    fail_on_20pct_performance_delta=False,
):
    """
    Give the metric values for two runs, augment the client/server dictionaries of the current
    run with the values from the compared run. Also compute difference (delta) and percentage
    of change (delta pctg) for each metric. Handle corner cases such as when a key/value is
    missing on the current or the compared run, when the difference is 0, or when the percentage
    change is 0. Round all results to the third decimal place. Finally, create the entries
    for the markdown comparison table

    Args:
        client_dictionary (dict):   The dictionary containing the metrics key-value pairs for the
                                    client from the current run
        server_dictionary (dict):   The dictionary containing the metrics key-value pairs for the
                                    server from the current run
        compared_client_dictionary (dict):  The dictionary containing the metrics key-value pairs
                                            for the client from the compared run
        compared_server_dictionary (dict):  The dictionary containing the metrics key-value pairs
                                            for the server from the compared run
        fail_on_20pct_performance_delta (bool): Whether to trigger an error if any of the most
                                                interesting metrics shows a performance delta that
                                                is larger than 20% in absolute value
    Returns:
        table_entries (list):   A list with two sublists. The two sublists contain the entries for
                                the markdown comparison tables for the client and server, respectively.
                                Each item in the sublists corresponds to one row in the table.
    """
    metrics_dictionaries = [client_dictionary, server_dictionary]
    compared_dictionaries = [compared_client_dictionary, compared_server_dictionary]
    table_entries = []

    test_result = "success"

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
            ]

            # Add the average value for the compared run
            if dictionary[k]["compared_avg"] == "N/A":
                new_entry.append("N/A")
            else:
                new_entry.append(f"{dictionary[k]['compared_avg']:.3f}")

            # Add the average value for the current run
            new_entry.append(f"{dictionary[k]['avg']:.3f}")

            # Add the difference
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
                if abs(dictionary[k]["delta_pctg"]) > 0.2:
                    if k in most_interesting_metrics and fail_on_20pct_performance_delta:
                        test_result = "failure (performance change on key metric >= 20%)"
                    emoji_delta = "❌"
                else:
                    emoji_delta = "✅"

            new_entry.append(emoji_delta)
            new_entry.append(dictionary[k]["plots"])
            new_table_entries.append(new_entry)
        table_entries.append(new_table_entries)

    return table_entries[0], table_entries[1], test_result


def generate_plots(plot_data_filename, branch_name, destination_folder, name_prefix, compared_plot_data_filename="", compared_branch_name="", verbose=False):
    """
    Generate the time-series plot of each metric in the file at the plot_data_filename path, and save
    each plot in the given destination_folder. The name of each plot will be destination_folder/<METRIC>.png

    Args:
        plot_data_filename (str):   The path to the file (usually <{client,server}>/plot_data.json) containing
                                the data needed for plotting
        destination_folder (str):   The path to the folder (usually <{client,server}>/plots) where we should
                                    save all the plots
        name_prefix (str): Prefix to add to the plots output files (e.g. info about experiment number, client/server)
        verbose (bool): Whether to print verbose logs to stdout
    Returns:
        None
    """

    if not os.path.isfile(plot_data_filename):
        print(f"Could not plot metrics because file {plot_data_filename} does not exist")
        return
    plot_data_file = open(plot_data_filename, "r")
    plot_data = json.loads(plot_data_file.read())
    plot_data_file.close()
    compared_plot_data = {}
    if len(compared_plot_data_filename) > 0 and os.path.isfile(compared_plot_data_filename) and len(compared_branch_name) > 0:
        compared_plot_data_file = open(plot_data_filename, "r")
        compared_plot_data = json.loads(compared_plot_data_file.read())
        compared_plot_data_file.close()

    for k in plot_data.keys():
        for trimmed_plot in (True, False):
            plt.figure()
            plt.plot(zip(plot_data[k]), label=branch_name)
            if k in compared_plot_data:
                plt.plot(zip(compared_plot_data[k]), label=compared_branch_name)
            plt.title(k)
            plt.xlabel("Time (s)")
            plt.ylabel("Value")
            if trimmed_plot:
                plt.xlim(left=5.0)
            plt.legend()
            plt.tight_layout()
            plt.grid(True)
            output_filename = os.path.join(
                destination_folder,
                f"{name_prefix}:{k}.png" if not trimmed_plot else f"{name_prefix}:{k}_trimmed.png",
            )
            plt.savefig(output_filename, dpi=300, bbox_inches="tight")


    def plot_metric(key, destination_folder, name_prefix, trimmed_plot, verbose):
        output_filename = os.path.join(
            destination_folder,
            f"{name_prefix}:{key}.png" if not trimmed_plot else f"{name_prefix}:{key}_trimmed.png",
        )
        time_range = f"0.0~36000.0" if not trimmed_plot else f"5.0~36000.0"
        plotting_command = f'python3 ../whist/debug/plotter.py -f "{k}" -r {time_range} -o {output_filename} {plot_data_filename}'
        if not verbose:
            subprocess.run(
                plotting_command, shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT
            )
        else:
            p = subprocess.run(plotting_command, shell=True, capture_output=True)
            if p.returncode != 0:
                print(f"Could not generate plot {output_filename}!")

    
    with open(plot_data_filename) as plot_data_file:
        data_file = json.loads(plot_data_file.read())
        for k in data_file.keys():
            for trimmed_plot in (True, False):
                plot_metric(
                    k, destination_folder, name_prefix, trimmed_plot=trimmed_plot, verbose=verbose
                )


def add_plot_links(
    client_metrics, server_metrics, plots_folder, plots_name_prefix, git_username, gist_id
):
    """
    Augment the client and server metrics with links to the time series plots. For each metric, we check
    if a full plot, a trimmed plot or both are available, and if so, we create a key:value pair where the
    value is a string with the Markdown hyperlinks to the plots.

    Args:
        client_metrics (dict):  The dictionary containing the metrics key-value pairs for the
                                client from the current run
        server_metrics (dict):  The dictionary containing the metrics key-value pairs for the
                                server from the current run
        plots_name_prefix (str): Prefix used to name all plot files
        git_username (str): The username corresponding to the token used to create the Gist
        gist_id (str): The ID of the Gist where we are uploading the plots and storing the reports with the results
    Returns:
        client_metrics (dict):  The dictionary containing the metrics key-value pairs for the
                                client from the current run, augmented with the links to the plots
        server_metrics (dict):  The dictionary containing the metrics key-value pairs for the
                                server from the current run, augmented with the links to the plots
    """

    def generate_plot_links(role, metric_key_name, git_username, gist_id):
        plot_links = []
        for plot_type in [("Full", ""), ("Trimmed", "_trimmed")]:
            readable_type, suffix_type = plot_type
            plot_name = f"{plots_name_prefix}:{role}:{metric_key_name}{suffix_type}.png"
            if os.path.isfile(os.path.join(plots_folder, plot_name)):
                plot_link = (
                    f"https://gist.githubusercontent.com/{git_username}/{gist_id}/raw/{plot_name}"
                )
                plot_links.append(f"[{readable_type}]({plot_link})")
        return ", ".join(plot_links)

    for metrics_tuples in (("client", client_metrics), ("server", server_metrics)):
        role, metrics_dictionary = metrics_tuples
        for k in metrics_dictionary:
            metrics_dictionary[k]["plots"] = generate_plot_links(role, k, git_username, gist_id)

    return client_metrics, server_metrics
