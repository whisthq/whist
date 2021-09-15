import psycopg2


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


def get_instance_ids(database_url):
    """
    Gets all aws instance ids from the database url

    Args:
        database_url (str): current database url

    Returns:
        arr: array of instance ids
    """
    query = "SELECT cloud_provider_id FROM instance_info;"
    ids = execute_db_query(database_url, "hardware", query)

    return [id[0] for id in ids]


def get_host_service_unresponsive_instances(database_url):
    """
    Gets all aws instance ids from the database url which have the status HOST_SERVICE_UNRESPONSIVE

    Args:
        database_url (str): current database url

    Returns:
        arr: array of tuples containing instance id and region
    """

    query = "SELECT cloud_provider_id, location FROM instance_info WHERE status='HOST_SERVICE_UNRESPONSIVE';"
    ids = execute_db_query(database_url, "hardware", query)

    return ids
