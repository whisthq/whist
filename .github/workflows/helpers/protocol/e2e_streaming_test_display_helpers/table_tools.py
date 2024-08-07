#!/usr/bin/env python3

import os
import sys
from pytablewriter import MarkdownTableWriter
from contextlib import redirect_stdout

# add the current directory to the path no matter where this is called from
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "."))

# Constants
N_NETWORK_CONDITION_PARAMETERS = 5

from protocol.e2e_streaming_test_display_helpers.metrics_tools import (
    generate_comparison_entries,
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

    if len(network_conditions.split(",")) == N_NETWORK_CONDITION_PARAMETERS:
        human_readable_network_conditions = network_conditions.split(",")

        def parse_value_or_range(raw_string, convert_string):
            raw_string = raw_string.split("-")
            if len(raw_string) == 1:
                return convert_string(raw_string[0])
            elif len(raw_string) == 2:
                return f"variable between {convert_string(raw_string[0])} and {convert_string(raw_string[1])}"
            else:
                print(
                    f"Error, network condition string '{raw_string}' contains incorrect number of values"
                )
                sys.exit(-1)

        transform_bandwidth_string = (
            lambda raw_string: raw_string if raw_string != "None" else "full available"
        )
        transform_delay_string = (
            lambda raw_string: raw_string + " ms" if raw_string != "None" else raw_string
        )
        transform_packet_drops_string = (
            lambda raw_string: "{:.2f}%".format(float(raw_string))
            if raw_string != "None"
            else raw_string
        )
        transform_packet_limit_string = (
            lambda raw_string: raw_string + " packets" if raw_string != "None" else raw_string
        )
        transform_interval_string = (
            lambda raw_string: raw_string + " ms" if raw_string != "None" else raw_string
        )

        bandwidth = parse_value_or_range(
            human_readable_network_conditions[0], transform_bandwidth_string
        )
        delay = parse_value_or_range(human_readable_network_conditions[1], transform_delay_string)
        packet_drops = parse_value_or_range(
            human_readable_network_conditions[2], transform_packet_drops_string
        )
        packet_limit = parse_value_or_range(
            human_readable_network_conditions[3], transform_packet_limit_string
        )
        interval = parse_value_or_range(
            human_readable_network_conditions[4], transform_interval_string
        )
        interval = "No." if interval == "None" else f"Yes, frequency is {interval}"
        human_readable_network_conditions = f"Bandwidth: {bandwidth}, Delay: {delay}, Packet Drops: {packet_drops}, Queue Limit: {packet_limit}, Conditions change over time? {interval}"

    elif network_conditions == "normal":
        human_readable_network_conditions = (
            f"Bandwidth: Unbounded, Delay: None, Packet Drops: None, Queue limit: default"
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
                    headers=["Metric", "Entries", "Average", "Plots"],
                    value_matrix=[
                        [
                            k,
                            metrics[k]["entries"],
                            f"{metrics[k]['avg']:.3f}",
                            metrics[k]["plots"],
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
    current_branch_name,
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
        current_branch_name (str): The name of the current branch
        compared_branch_name (str): The name of the branch that we are comparing against
        most_interesting_metrics (set): list of metrics that we want to display at the top (if found)
        client_metrics (dict):  the client key-value pairs with metrics from the current run
        server_metrics (dict):  the server key-value pairs with metrics from the current run
        compared_client_metrics (dict): the client key-value pairs with metrics from the compared run
        compared_server_metrics (dict): the server key-value pairs with metrics from the compared run

    Returns:
        None
    """

    client_table_entries, server_table_entries, test_result = generate_comparison_entries(
        client_metrics,
        server_metrics,
        compared_client_metrics,
        compared_server_metrics,
        most_interesting_metrics,
        fail_on_20pct_performance_delta=(
            current_branch_name != "dev" and compared_branch_name == "dev"
        ),
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
                        f"Average ({compared_branch_name})",
                        "Average (this branch)",
                        "Delta",
                        "",
                        "Plots",
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

    return test_result
