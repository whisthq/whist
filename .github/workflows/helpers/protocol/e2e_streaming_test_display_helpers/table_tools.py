#!/usr/bin/env python3

import os
import sys
from pytablewriter import MarkdownTableWriter
from contextlib import redirect_stdout
from datetime import datetime, timedelta

# add the current directory to the path no matter where this is called from
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "."))

from protocol.e2e_streaming_test_display_helpers.metrics_tools import (
    compute_deltas,
)


def generate_no_comparison_table(
    results_file, experiment_metadata, most_interesting_metrics, client_metrics, server_metrics
):
    """
    Create a Markdown table to display the client and server metrics for the current run.
    Also include a table with the metadate of the run, and a table with the most interesting
    metrics. Do not include comparisons with other runs.
    Args:
        results_file (file): The open file where we want to save the markdown table
        experiment_metadata (dict): The metadata key-value pairs for the current run
        most_interesting_metrics (list): list of metrics that we want to display at the top (if found)
        client_metrics (dict): the client key-value pairs for the current run
        server_metrics (dict): the server key-value pairs for the current run
    Returns:
        None
    """
    experiment_metrics = [
        {"name": "CLIENT", "metrics": client_metrics},
        {"name": "SERVER", "metrics": server_metrics},
    ]
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
        results_file (file): The open file where we want to save the markdown table
        most_interesting_metrics (list): list of metrics that we want to display at the top (if found)
        experiment_metadata (dict): The metadata key-value pairs for the current run
        compared_experiment_metadata (dict): The metadata key-value pairs for the compared run
        client_table_entries (list): the table entries for the client table
        server_table_entries (list): the table entries for the server table
        branch_name (str): the name of the branch for the compared run
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
