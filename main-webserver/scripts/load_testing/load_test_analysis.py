"""
Analyze a load test. All JSONs saved to `OUTFOLDER`. Can handle
both a local load test or a distributed load test. Main functions
of interest:
- analyze_load_test
"""

import sys
import os
import json
from typing import List, Dict, Union
import math

# this adds the load_testing folder root to the python path no matter where
# this file is called from. We can now import from `scripts`.
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "../.."))

import pandas as pd

from scripts.load_testing.load_test_utils import OUTFOLDER
from scripts.load_testing.load_test_models import initialize_db, dummy_flask_app, db, LoadTesting


def load_load_test_data():
    """
    TODO

    Returns:
        List of dictionaries structured as described above
    """
    exc_path = os.path.join(OUTFOLDER, "locust_load_test_exceptions.csv")
    fail_path = os.path.join(OUTFOLDER, "locust_load_test_failures.csv")
    stats_path = os.path.join(OUTFOLDER, "locust_load_test_stats.csv")
    assert os.path.exists(exc_path)
    assert os.path.exists(fail_path)
    assert os.path.exists(stats_path)

    df_exc = pd.read_csv(exc_path)
    df_stats = pd.read_csv(stats_path)
    return df_exc, df_stats


def analyze_load_test(run_id: str = None, db_uri: str = None):
    """
    Analyze a load test by inspecting the files in OUTFOLDER. First we check for errors.
    If there are some, we print them and error out. Otherwise, we compute the following stats:
    - max web request time
    - avg web request time
    - std web request time
    - max container time
    - avg container time
    - std container time

    Args:
        run_id: The id of this load testing run. In workflows, just use the workflow id. If None,
            nothing will be saved to the db.
        db_uri: Must be provided if `run_id` is not None. Allows the script to save the test
            results at `db_uri`.
    """
    if run_id is None:
        print("Warning. No run_id was passed, so results will not be saved to the db.")
    else:
        assert db_uri is not None, "run_id is not None so db_uri must be given"
        initialize_db(db_uri)

    df_exc, df_stats = load_load_test_data()
    # assert len(df_exc) == 0, f"Got failures:\n{df_exc}"

    status_rows = df_stats[df_stats["Name"].apply(lambda x: x.startswith("/status"))]
    print(status_rows[["Name", "Request Count", "Requests/s"]])
    num_users = len(status_rows)
    # pd.Series list of how long it took each load test user to get a container
    all_user_container_times = status_rows["Request Count"] / status_rows["Requests/s"]
    print(all_user_container_times)
    avg_container_time = all_user_container_times.mean()
    max_container_time = all_user_container_times.max()
    std_container_time = all_user_container_times.std() if num_users > 1 else 0
    user_errors = df_stats["Failure Count"].sum()

    aggregate_row = df_stats[df_stats["Name"] == "Aggregated"]
    # convert msec to sec
    avg_web_time = aggregate_row["Average Response Time"].values[0] / 1000
    max_web_time = aggregate_row["Max Response Time"].values[0] / 1000

    new_lt_entry = LoadTesting(
        run_id=run_id,
        num_users=num_users,
        avg_container_time=avg_container_time,
        max_container_time=max_container_time,
        std_container_time=std_container_time,
        avg_web_time=avg_web_time,
        max_web_time=max_web_time,
        user_errors=user_errors,
    )

    print(f"Load Test summary:\n{new_lt_entry}")

    if run_id is None:
        return

    with dummy_flask_app.app_context():
        db.session.add(new_lt_entry)
        db.session.commit()

    print(f"Saved new load testing entry for run {run_id}")
