#!/usr/bin/env python3

import sys
import os
import psycopg2


# add the current directory to the path no matter where this is called from
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "."))


def execute_db_query(database_url, path, query):
    """
    Executes a given query based on the database url and path

    Args:
        database_url (str): current database url
        path (str): the search path
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
    query = "SELECT cloud_provider_id FROM instance_info WHERE location='%s';" % region
    ids = execute_db_query(database_url, "hardware", query)

    return [id[0] for id in ids]


def get_host_service_unresponsive_instance_ids(database_url, region):
    """
    Gets all aws instance ids using the database url and region which have the status HOST_SERVICE_UNRESPONSIVE

    Args:
        database_url (str): current database url
        region (str): current region

    Returns:
        arr: array of instance ids
    """

    query = (
        "SELECT cloud_provider_id FROM instance_info WHERE status='HOST_SERVICE_UNRESPONSIVE' AND location = '%s';"
        % region
    )
    ids = execute_db_query(database_url, "hardware", query)

    return [id[0] for id in ids]
