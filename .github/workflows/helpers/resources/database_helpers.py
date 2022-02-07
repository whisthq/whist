#!/usr/bin/env python3

import sys
import os
import psycopg2

from datetime import datetime


# add the current directory to the path no matter where this is called from
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "."))


def execute_db_query(database_url, path, query):
    """
    Executes a given query based on the database url and path

    Args:
        database_url (str): current database url
        path (str): the search path (schema)
        query (str): query to be executed

    Returns:
        arr: result of executing query
    """
    conn = psycopg2.connect(database_url, sslmode="require")

    cur = conn.cursor()

    cur.execute("SET search_path TO %s;" % path)
    cur.execute("%s;" % query)

    return cur.fetchall()


def get_instance_ids(database_url, region):
    """
    Gets all aws instance ids using the database url and region

    Args:
        database_url (str): current database url
        region (str): current region

    Returns:
        arr: array of instance ids
    """
    query = "SELECT id FROM instances WHERE region='%s';" % region
    ids = execute_db_query(database_url, "whist", query)

    return [id[0] for id in ids]


def get_lingering_instances(database_url, region):
    """
    Gets all aws instance ids and names using the database url and region which have the status DRAINING

    Args:
        database_url (str): current database url
        region (str): current region

    Returns:
        arr: array of tuples (name, id)
    """

    query = (
        "SELECT id, updated_at FROM instances WHERE status='DRAINING' AND region = '%s';" % region
    )
    instances = execute_db_query(database_url, "whist", query)

    lingering_instances = []

    for instance in instances:
        curr_time = datetime.now()
        last_updated_time = instance[1]

        # Get difference in minutes
        minutes_diff = (curr_time - last_updated_time).total_seconds() / 60.0

        # If the instance has been draining for more than 10 minutes, consider as lingering
        if minutes_diff > 10:
            lingering_instances.append(instance)

    return lingering_instances
