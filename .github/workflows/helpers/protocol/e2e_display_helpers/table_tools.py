#!/usr/bin/env python3

import os
import sys
from pytablewriter import MarkdownTableWriter
from contextlib import redirect_stdout
from datetime import datetime, timedelta

# add the current directory to the path no matter where this is called from
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "."))


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
                headers=["Metric", "Entries", "Average", "Min", "Max"],
                value_matrix=[
                    [
                        k,
                        interesting_metrics[k]["entries"],
                        f"{interesting_metrics[k]['avg']:.3f}",
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
            headers=["Metric", "Entries", "Average", "Min", "Max"],
            value_matrix=[
                [
                    k,
                    client_metrics[k]["entries"],
                    f"{client_metrics[k]['avg']:.3f}",
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
            headers=["Metric", "Entries", "Average", "Min", "Max"],
            value_matrix=[
                [
                    k,
                    server_metrics[k]["entries"],
                    server_metrics[k]["avg"],
                    server_metrics[k]["min"],
                    server_metrics[k]["max"],
                ]
                for k in server_metrics
            ],
            margin=1,  # add a whitespace for both sides of each cell
            max_precision=3,
        )
        writer.write_table()

        print("\n")
        print("</details>")
        print("\n")


def generate_comparison_table(
    results_file,
    most_interesting_metrics,
    experiment_metadata,
    compared_experiment_metadata,
    client_table_entries,
    server_table_entries,
    branch_name,
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
                    "Average (this branch)",
                    f"Average ({branch_name})",
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
        if len(client_table_entries) == 0:
            print("NO CLIENT METRICS\n")
        else:
            print("###### CLIENT METRICS: ######\n")

        writer = MarkdownTableWriter(
            # table_name="Client metrics",
            headers=[
                "Metric",
                "Entries (this branch)",
                "Average (this branch)",
                f"Average ({branch_name})",
                "Delta",
                "",
            ],
            value_matrix=[i for i in client_table_entries],
            margin=1,  # add a whitespace for both sides of each cell
            max_precision=3,
        )
        writer.write_table()
        print("\n")

        if len(server_table_entries) == 0:
            print("NO SERVER METRICS\n")
        else:
            print("###### SERVER METRICS: ######\n")

        # Generate server table
        writer = MarkdownTableWriter(
            # table_name="Server metrics",
            headers=[
                "Metric",
                "Entries (this branch)",
                "Average (this branch)",
                f"Average ({branch_name})",
                "Delta",
                "",
            ],
            value_matrix=[i for i in server_table_entries],
            margin=1,  # add a whitespace for both sides of each cell
            max_precision=3,
        )
        writer.write_table()

        print("\n")
        print("</details>")
        print("\n")
