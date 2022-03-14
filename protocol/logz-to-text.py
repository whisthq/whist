#!/usr/bin/env python3

"""
The file, given a session id parameter, will obtain all the logs present in
logz.io for that session id.

This file uses the Logz.io SCROLL API, whose documentation is below:
@see https://docs.logz.io/api/#operation/scroll

Basically, using this API, we can send an initial POST request with a query
for the logs we desire.

In the initial request, we need to provide a header and a body.

The header will be our "X_API_TOKEN", which is the `logz-to-text.py-token` API token, and the
"Content-Type", which will always be "application/json".

The body will contatin the meat of the request. There are 3 fields we need
to fill in the body:

1. query: This describes what kind of logs you want to recieve. The docs
on how to make this are terrible, so I recommend that you do the search manually
on logz.io and then navigate to the "Inpect" tab. This will show you the resulting
query that the website made. Now, copy that over. More detail provided in the
comments inside the "init_scrolling" function in this file.

2. _sources: What fields from the logs we want to recieve. Here, we only
care about the message (the actual log data produced from logging.c),
the timestamp (timestamp logz.io provides to the logs when they recieve them),
and component (which is either clientapp or mandlebox, which indicates that
this is a log for the client or server, respectively).

3. size: How many logs per pagination to recieve.

In the initial reply, we will recieve 2 bigs pieces of info:

1. "hits": These are the total number of logs we will recieve. This will be
used to determine how many times we need to paginate
2. "scroll_id": This is required in following POST requests to get the next
page of results (where the number of logs in each POST is LOGS_PER_PAGE long).

Then, we will paginate with that same SCROLL_ID until the logs recieved is
equal to the number of logs Logz.io has. Note, we do not need to resend the query
each time. Only the header and the scroll_id.

"""

import requests
import sys
import json
import os, os.path
import errno
from datetime import datetime, timedelta
from tqdm import tqdm


# API URL
LOGZ_IO_SCROLL_URL = "https://api.logz.io/v1/scroll"

# How many logs per pagination we will get
LOGS_PER_PAGE = 1000

# Number of days Logz.io retains the logs
RETENTION_PERIOD_DAYS = 7

# Account API keys for base, dev, staging, & prod Logz.io servers
# Named "logz-to-text.py-token" on each account
ACCOUNT_KEY_PAIRS = {
    ("base", "f79b29bf-187d-4051-a4c9-f7273fd471fd"),
    ("dev", "32be01be-1e83-41c8-8997-908d74823173"),
    ("staging", "23af72c5-04ed-4eaf-9e02-c5ef093fc195"),
    ("prod", "606a4195-dc8e-439e-967b-cb8413714944"),
}

# Errors from 'requests' are ugly, let's try to reduce the likely-hood
# of those errors occuring
def validate_args(argv):
    if len(argv) != 2:
        raise Exception("Usage: python3 logz-to-text.py SESSION_ID")


# Gets a time 'days' days from current time in ElasticSearch timestamp format
# To get the current time in ElasticSearch timestamp format, pass in 0
# when calling this function
def get_time_from_now_in_days_formatted(days):
    now = datetime.isoformat(datetime.utcnow() - timedelta(days=days))
    return now + "Z"


# Sends the initial request to the POST Logz.io scroll API
# to establish the query (in this case, what session_id to look for)
# and how many results per page (LOGS_PER_PAGE) to return.
# Furthermore, also used to get the scroll_id that is needed to paginate
# the results
def init_scrolling(headers, query_string):
    # The documentation from logz.io is horrible, so the way I found to
    # construct the query was from this link: https://docs.logz.io/api/cookbook/inspect.html
    # Basically, manually search your query on the logz.io website, then
    # use the inspect feature to see what query the website generates,
    # then copy that query into here.
    # This is technically for the search API (we are using the Logz.io scroll API)
    # but the query is the same for both

    current_timestamp = get_time_from_now_in_days_formatted(0)
    beginning_timestamp = get_time_from_now_in_days_formatted(RETENTION_PERIOD_DAYS)

    initial_post_body = {
        "query": {
            "bool": {
                "must": [{"match_all": {}}],
                "filter": [
                    {"match_phrase": {"session_id": query_string}},
                    {
                        "range": {
                            "@timestamp": {
                                "gte": beginning_timestamp,  # beginning time range to get logs
                                "lte": current_timestamp,  # ending time range to get logs
                                "format": "strict_date_optional_time",  # format needed for elastic search API to parse timestamps
                            }
                        }
                    },
                ],
            }
        },
        # Just says what to include with the logs
        # Message is our logging message and @timestamp is the logz.io timestamp
        # of when the log was updated
        "_source": {"includes": ["message", "@timestamp", "component"]},
        # Number of logs to provide per pagination
        "size": LOGS_PER_PAGE,
    }

    response = requests.post(
        url=LOGZ_IO_SCROLL_URL, data=json.dumps(initial_post_body), headers=headers
    )

    content = response.json()

    return content["scrollId"], json.loads(content["hits"])["hits"]


# Gets the page of logs from a POST request to Logz.io scroll API
def get_logs_page(headers, scroll_id):
    paginate_body = {"scroll_id": scroll_id}

    response = requests.post(
        url=LOGZ_IO_SCROLL_URL, data=json.dumps(paginate_body), headers=headers
    )

    content = json.loads(response.json()["hits"])

    return content["total"], content["hits"]


# This will sort the logs by date and time
def sort_logs(parsed_logs):
    return parsed_logs.sort(key=lambda log: log["timestamp"])


# Parses all the logs from the log_page
# Adds them into array paired with it's timestamp (from logz.io)
# for later sorting purposes
def parse_logs(parsed_logs, logs_page):
    for log in logs_page:
        try:
            message = log["_source"]["message"]
        except:
            continue
        # Each timestamp comes something like "2021-10-22T15:05:05.420Z"
        # and we only need the "2021-10-22", which is the first 9 characters
        logz_io_timestamp = log["_source"]["@timestamp"]
        year_month_day = logz_io_timestamp[:10]

        # We need the component to separate out server vs client logs
        component = log["_source"]["component"]


        try:
            message_start = message.split()[0]
            timestamp = datetime.fromisoformat(message_start)
        except ValueError:
            logz_io_timestamp = log["_source"]["@timestamp"]
            # Strip the trailing 'Z' from logz_io_timestamp
            timestamp = datetime.fromisoformat(logz_io_timestamp.strip("Z"))

        if component == "clientapp":
            # if it's client app, then we know these are logs from the client
            origin = "client"
        else:
            # otherwise, this is mandlebox, so these are logs from the server
            origin = "server"
        parsed_logs.append({ "origin": origin, "timestamp": timestamp, "message": message })

# Writes the server and client logs to two separate files
# File names are SESSION_ID-client.log and SESSION_ID-server.log
def write_logs_to_files(parsed_logs, session_id, account):

    client_logs_file_name = "{}-client-{}.log".format(session_id, account)
    server_logs_file_name = "{}-server-{}.log".format(session_id, account)

    # Get download directory, relative to the protocol directory
    # If not filled in, then it will default to the /protocol folder
    download_folder = os.getenv("LOGS_DOWNLOAD_FOLDER", os.getcwd())

    client_file_path = (
        os.path.join(download_folder, client_logs_file_name)
        if download_folder is not None
        else client_logs_file_name
    )
    server_file_path = (
        os.path.join(download_folder, server_logs_file_name)
        if download_folder is not None
        else server_logs_file_name
    )

    # Log folder for transparency
    print("Download folder: " + download_folder)

    # exit()
    client_file = open(client_file_path, "w")
    server_file = open(server_file_path, "w")
    for log in parsed_logs:
        if log["origin"] == "client":
            client_file.write(log["message"])
            client_file.write("\n")
        else:
            server_file.write(log["message"])
            server_file.write("\n")
    server_file.close()
    client_file.close()


def scan_account(session_id, account, api_key):
    # Headers to authorize Logz.io scroll API in each account
    headers = {"X-API-TOKEN": api_key, "Content-Type": "application/json"}

    # We need to get the initial scroll_id as described in the docs to
    # paginate the results.
    # This function also returns the first page of paginated results,
    # as described in the docs
    parsed_logs = []
    scroll_id, logs_page = init_scrolling(headers, session_id)
    parse_logs(parsed_logs, logs_page)

    # This continues to paginate, making POST requests to the Logz.io scroll API
    # and adds them to the parsed_logs array until there are no more results
    total, logs_page = get_logs_page(headers, scroll_id)

    # Couldn't find logs
    if len(logs_page) == 0:
        print("No logs for {} in {}".format(session_id, account))
        return

    print("Found logs for {} in {}".format(session_id, account))
    for i in tqdm(range(total, 0, -len(parsed_logs)), desc="Scanning logs in {}".format(account)):
        # This will take a bit, so it's nice to have some sort of progress meter
        # so the user doesn't think they're program is crashing
        # print(total - len(parsed_logs), " logs remaining")
        parse_logs(parsed_logs, logs_page)
        total, logs_page = get_logs_page(headers, scroll_id)

    # Sort the logs by date and time
    sort_logs(parsed_logs)

    # Write results- separated by a new line- to a log file
    # named "{SESSION_ID}-{client/server}-{base/dev/staging/prod}.log"
    write_logs_to_files(parsed_logs, session_id, account)


if __name__ == "__main__":
    # Best to inform users of boo-boos before they are sent to APIs
    validate_args(sys.argv)
    session_id = sys.argv[1]

    # Scan all Logz.io servers for the session_id
    for account, api_key in ACCOUNT_KEY_PAIRS:
        scan_account(session_id, account, api_key)
