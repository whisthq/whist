#!/usr/bin/env python3
import requests
import sys
import json
from datetime import datetime

# API key needed for authentication
LOGZ_IO_API_KEY = "948fd3f8-43b5-429a-a8e0-61f123beb79f"

# API URL
LOGZ_IO_SCROLL_URL = "https://api.logz.io/v1/scroll"

# How many logs per pagination we will get
LOGS_PER_PAGE = 1000

# Post headers needed for authentication
LOGZ_IO_HEADERS = {"X-API-TOKEN": LOGZ_IO_API_KEY, "Content-Type": "application/json"}

# Errors from 'requests' are ugly, let's try to reduce the likely-hood
# of those errors occuring
def validate_args(argv):
    if len(argv) != 2:
        raise Exception("Usage: python3 logs-to-text.py <<session_id>>")


# Sends the initial request to the POST logz.io scroll API
# to establish the query (in this case, what session_id to look for)
# and how many results per page (LOGS_PER_PAGE) to return.
# Furthermore, also used to get the scroll_id that is needed to paginate
# the results
def init_scrolling(query_string):
    # The documentation from logz.io is horrible, so the way I found to
    # construct the query was from this link: https://docs.logz.io/api/cookbook/inspect.html
    # Basically, manually search your query on the logz.io website, then
    # use the inspect feature to see what query the website generates,
    # then copy that query into here.
    # This is technically for the search API (we are using the scroll API)
    # but the query is the same for both
    initial_post_body = {
        "query": {"bool": {"must": [{"query_string": {"query": query_string}}]}},
        # Just says what to include with the logs
        # Message is our logging message and @timestamp is the logz.io timestamp
        # of when the log was updated
        "_source": {"includes": ["message", "@timestamp"]},
        "size": LOGS_PER_PAGE,
    }

    response = requests.post(
        url=LOGZ_IO_SCROLL_URL, data=json.dumps(initial_post_body), headers=LOGZ_IO_HEADERS
    )

    content = response.json()

    return content["scrollId"], json.loads(content["hits"])["hits"]


# Gets the page of logs from a POST request to logz.io scroll API
def get_logs_page(scroll_id):
    paginate_body = {"scroll_id": scroll_id}

    response = requests.post(
        url=LOGZ_IO_SCROLL_URL, data=json.dumps(paginate_body), headers=LOGZ_IO_HEADERS
    )

    content = json.loads(response.json()["hits"])

    return content["total"], content["hits"]


# This will sort the logs by date and time
def sort_logs(parsed_logs):
    return parsed_logs.sort(key=lambda log: datetime.strptime(log[:19], "%Y-%m-%d %H:%M:%S"))


# Parses all the logs from the log_page
# Adds them into array paired with it's timestamp (from logz.io)
# for later sorting purposes
def parse_logs(parsed_logs, logs_page):
    for log in logs_page:
        message = log["_source"]["message"]
        # Each timestamp comes something like "2021-10-22T15:05:05.420Z"
        # and we only need the "2021-10-22", which is the first 9 characters
        logz_io_timestamp = log["_source"]["@timestamp"]
        year_month_day = logz_io_timestamp[:10]

        # Sometimes, in errors, we do not include hour-month-day, so check
        # if the first character to see if it's a number.
        if not message[0].isnumeric():
            # if we do not provide an hour-minute-second,
            # use the hour-minute-second from logz_io timestamp
            hour_minute_second = logz_io_timestamp[11:19]
            message = year_month_day + " " + hour_minute_second + " | " + message
        else:
            # otherwise, use our hour-minute-second
            hour_minute_second = message[:8]
            message = year_month_day + " " + hour_minute_second + message[15:]

        parsed_logs.append(message)


if __name__ == "__main__":
    # Best to inform users of boo-boos before they are sent to APIs
    validate_args(sys.argv)
    session_id = sys.argv[1]

    # We need to get the initial scroll_id as described in the docs to
    # paginate the results.
    # This function also returns the first page of paginated results,
    # as described in the docs
    parsed_logs = []
    scroll_id, logs_page = init_scrolling(session_id)
    parse_logs(parsed_logs, logs_page)

    # This continues to paginate, making POST requests to the scroll API
    # and adds them to the parsed_logs array until there are no more results
    total, logs_page = get_logs_page(scroll_id)
    while len(logs_page) != 0:
        # This will take a bit, so it's nice to have some sort of progress meter
        # so the user doesn't think they're program is crashing
        print(total - len(parsed_logs), " logs remaining")
        parse_logs(parsed_logs, logs_page)
        total, logs_page = get_logs_page(scroll_id)

    # Sort the logs by date and time
    sort_logs(parsed_logs)

    # Write results- separated by a new line- to  a text file
    # named "<<session_id>>_logs.txt"
    file_name = "{}_logs.txt".format(session_id)
    file = open(file_name, "w")
    for log in parsed_logs:
        file.write(log)
        file.write("\n")
    file.close()
