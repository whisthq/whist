#!/usr/bin/env python3
"""
Written by Hamish Nicholson 2020/08/11
This file downloads and extracts the latest sentry-native release direct from their github
"""

import requests
from zipfile import ZipFile
import argparse

def get_sentry(token=None):
    try:
        headers = {}
        if token:
            headers = {'Authorization': 'token %s' % token}
        response = requests.get("https://api.github.com/repos/getsentry/sentry-native/releases/latest", headers=headers)
        download_url = response.json()['assets'][0]['browser_download_url']
        r = requests.get(download_url, allow_redirects=True)
        open('sentry-native.zip', 'wb+').write(r.content)
    except:
        print("Failed to download sentry.")
        print("Headers: ", response.headers)
        print("Response: ", response.json())

    with ZipFile('sentry-native.zip', 'r') as zipObj:
        # Extract all the contents of zip file in current directory
        zipObj.extractall('sentry-native')


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )

    parser.add_argument(
        "--token",
        help="github PAT for optionally authenticated API requests",
        default=None
    )
    args = parser.parse_args()
    get_sentry(args.token)
