#!/usr/bin/env python3
import requests
import sys
import json

from requests.api import get

LOGZ_IO_API_KEY = '948fd3f8-43b5-429a-a8e0-61f123beb79f'
LOGZ_IO_SEARCH_URL = 'https://api.logz.io/v1/scroll'
LOGS_PER_PAGE = 1000
LOGZ_IO_HEADERS = {
    'X-API-TOKEN': LOGZ_IO_API_KEY,
    'Content-Type': 'application/json'
}


def validate_args(argv):
    if len(argv) != 2:
        raise Exception("Usage: python3 logs-to-text.py <<session_id>>")


def init_scrolling(query_string):
    initial_post_body = {
        "query": {
            "bool": {
                "must": [
                    {
                        "query_string": {
                            "query": query_string
                        }
                    }
                ]
            }
        },
        "_source": {
            "includes": [
                "message"
            ]
        },
        "size": LOGS_PER_PAGE
    }

    response = requests.post(url=LOGZ_IO_SEARCH_URL,
                             data=json.dumps(initial_post_body),
                             headers=LOGZ_IO_HEADERS)

    return response.json()['scrollId']


def get_logs_page(scroll_id):
    paginate_body = {
        "scroll_id": scroll_id
    }

    response = requests.post(url=LOGZ_IO_SEARCH_URL,
                             data=json.dumps(paginate_body),
                             headers=LOGZ_IO_HEADERS)
    
    content = json.loads(response.json()['hits'])

    return content['total'], content['hits']

def parse_logs(parsed_logs, logs_page):
    for log in logs_page:
        message = log['_source']['message']
        parsed_logs.append(message)


if __name__ == "__main__":
    validate_args(sys.argv)
    session_id = sys.argv[1]
    scroll_id = init_scrolling(session_id)
    
    parsed_logs = []
    total, logs_page = get_logs_page(scroll_id)
    while len(logs_page) != 0:
        print(total - len(parsed_logs), " logs remaining")
        parse_logs(parsed_logs, logs_page)
        total, logs_page = get_logs_page(scroll_id)
    
    file_name = '{}_logs.txt'.format(session_id)
    file = open(file_name, 'w')
    for log in parsed_logs:
        file.write(log)
        file.write('\n')
    file.close()

