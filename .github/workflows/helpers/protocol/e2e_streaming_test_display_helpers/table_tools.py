#!/usr/bin/env python3

import os
import sys
from pytablewriter import MarkdownTableWriter
from contextlib import redirect_stdout

# add the current directory to the path no matter where this is called from
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "."))

from protocol.e2e_streaming_test_display_helpers.metrics_tools import (
    compute_deltas,
)


def network_conditions_to_readable_form(network_conditions):
    """
    Creates a human-readable string to describe the network conditions used in the E2E test.

    Args:
        network_conditions (str):   the network conditions in a standardized, succinct format. The string is set
                                    to 'normal' if no artificial network degradations are applied, or 'unknown'
                                    if the experiment failed and/or if the network conditions are not known. Otherwise,
                                    network_conditions consists of three comma-separated values: the max bandwidth allowed,
                                    the delay, and the percentage of packet drops. A value of `none` indicates no degradation.

    Returns:
        human_readable_network_conditions (str):    the network conditions containing the same information as network_conditions
                                                    but in a human-readable format.
    """
    human_readable_network_conditions = network_conditions

    if len(network_conditions.split(",")) == 3:
        human_readable_network_conditions = network_conditions.split(",")
        bandwidth = (
            human_readable_network_conditions[0]
            if human_readable_network_conditions[0] != "none"
            else "full available"
        )
        delay = (
            human_readable_network_conditions[1] + " ms"
            if human_readable_network_conditions[1] != "none"
            else human_readable_network_conditions[1]
        )
        packet_drops = (
            "{:.2f}%".format(float(human_readable_network_conditions[2]) * 100.0)
            if human_readable_network_conditions[2] != "none"
            else human_readable_network_conditions[2]
        )
        human_readable_network_conditions = (
            f"Bandwidth: {bandwidth}, Delay: {delay}, Packet Drops: {packet_drops}"
        )
    elif network_conditions == "normal":
        human_readable_network_conditions = (
            f"Bandwidth: maximum available, Delay: none, Packet Drops: none"
        )

    return human_readable_network_conditions


def generate_metadata_table(results_file, experiment_metadata, compared_experiment_metadata=None):
    """
    Create a Markdown table to display the metadata of a run of the E2E test. Optionally, if the
    compared_experiment_metadata argument is defined, we also display the metadata of a compared run.

    Args:
        results_file (file):    The open file where we want to save the markdown table
        experiment_metadata (dict): The metadata key-value pairs for the run in question
        compared_experiment_metadata (dict): The metadata key-value pairs for a compared run

    Returns:
        None
    """
    with redirect_stdout(results_file):
        print("<details>")
        print("<summary>Experiment metadata - Expand here</summary>")
        print("\n")

        print("###### Experiment metadata: ######\n")

        if compared_experiment_metadata:
            # Generate metadata table with comparison
            writer = MarkdownTableWriter(
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
        else:
            # Generate metadata table without comparison
            writer = MarkdownTableWriter(
                headers=["Key (this run)", "Value (this run)"],
                value_matrix=[
                    [
                        k,
                        experiment_metadata[k],
                    ]
                    for k in experiment_metadata
                ],
                margin=1,  # add a whitespace for both sides of each cell
            )
            writer.write_table()

        print("\n")
        print("</details>")
        print("\n")


def generate_results_table(
    results_file, experiment_metadata, most_interesting_metrics, client_metrics, server_metrics
):
    """
    Create a Markdown table to display the client and server metrics for the current run.
    Also include a table with the metadata of the run, and a table with the most interesting
    metrics. Do not include comparisons with other runs.

    Args:
        results_file (file):    The open file where we want to save the markdown table
        experiment_metadata (dict): The metadata key-value pairs for the current run
        most_interesting_metrics (set): set of metrics that we want to display at the top (if found)
        client_metrics (dict):  the client key-value pairs with metrics from the current run
        server_metrics (dict):  the server key-value pairs with metrics from the current run

    Returns:
        None
    """
    experiment_metrics = [
        {"name": "CLIENT", "metrics": client_metrics},
        {"name": "SERVER", "metrics": server_metrics},
    ]

    # Generate metadata table
    generate_metadata_table(results_file, experiment_metadata, compared_experiment_metadata=None)

    with redirect_stdout(results_file):
        print("<details>")
        print("<summary>Experiment results - Expand here</summary>")
        print("\n")

        # Generate most interesting metric dictionary
        interesting_metrics = {}
        for item in experiment_metrics:
            for k in item["metrics"]:
                if k in most_interesting_metrics:
                    interesting_metrics[k] = item["metrics"][k]
        experiment_metrics.insert(0, {"name": "MOST INTERESTING", "metrics": interesting_metrics})

        # Generate tables for most interesting metrics, client metrics, and server metrics
        for item in experiment_metrics:
            name = item["name"]
            metrics = item["metrics"]

            if len(metrics) == 0:
                print(f"NO {name} METRICS\n")
            else:
                print(f"###### {name} METRICS: ######\n")

                writer = MarkdownTableWriter(
                    # table_name="Interesting metrics",
                    headers=["Metric", "Entries", "Average"],
                    value_matrix=[
                        [
                            k,
                            metrics[k]["entries"],
                            f"{metrics[k]['avg']:.3f}",
                        ]
                        for k in metrics
                    ],
                    margin=1,  # add a whitespace for both sides of each cell
                    max_precision=3,
                )
                writer.write_table()
                print("\n")

        print("\n")
        print("</details>")
        print("\n")


def generate_comparison_table(
    results_file,
    experiment_metadata,
    compared_experiment_metadata,
    compared_branch_name,
    most_interesting_metrics,
    client_metrics,
    server_metrics,
    compared_client_metrics,
    compared_server_metrics,
):
    """
    Create a Markdown table to display the client and server metrics for the current run,
    as well as those from a compared run. Also include a table with the metadata of the run,
    and a table with the most interesting metrics.

    Args:
        results_file (file):    The open file where we want to save the markdown table
        experiment_metadata (dict): The metadata key-value pairs for the current run
        compared_experiment_metadata (dict):    the metadata key-value pairs for the run we are
                                                comparing against
        compared_branch_name (str): The name of the branch that we are comparing against
        most_interesting_metrics (set): list of metrics that we want to display at the top (if found)
        client_metrics (dict):  the client key-value pairs with metrics from the current run
        server_metrics (dict):  the server key-value pairs with metrics from the current run
        compared_client_metrics (dict): the client key-value pairs with metrics from the compared run
        compared_server_metrics (dict): the server key-value pairs with metrics from the compared run

    Returns:
        None
    """

    client_table_entries, server_table_entries = compute_deltas(
        client_metrics, server_metrics, compared_client_metrics, compared_server_metrics
    )

    table_entries = [
        {"name": "CLIENT", "metrics": client_table_entries},
        {"name": "SERVER", "metrics": server_table_entries},
    ]

    # Generate metadata table
    generate_metadata_table(results_file, experiment_metadata, compared_experiment_metadata)

    with redirect_stdout(results_file):

        print("<details>")
        print("<summary>Experiment results - Expand here</summary>")
        print("\n")

        # Generate most interesting metric dictionary
        interesting_metrics = [
            row
            for item in table_entries
            for row in item["metrics"]
            if row[0] in most_interesting_metrics
        ]
        table_entries.insert(0, {"name": "MOST INTERESTING", "metrics": interesting_metrics})

        # Generate tables for most interesting metrics, client metrics, and server metrics
        for item in table_entries:
            name = item["name"]
            entries = item["metrics"]
            if len(entries) == 0:
                print(f"NO {name} METRICS\n")
            else:
                print(f"###### SUMMARY OF {name} METRICS: ######\n")

                writer = MarkdownTableWriter(
                    # table_name="Interesting metrics",
                    headers=[
                        "Metric",
                        "Entries (this branch)",
                        "Average (this branch)",
                        f"Average ({compared_branch_name})",
                        "Delta",
                        "",
                    ],
                    value_matrix=[i for i in entries],
                    margin=1,  # add a whitespace for both sides of each cell
                    max_precision=3,
                )
                writer.write_table()
                print("\n")

        print("\n")
        print("</details>")
        print("\n")
