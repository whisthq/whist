#!/usr/bin/env python3
"""
Written by Hamish Nicholson 2020/08/11
This file downloads and extracts the latest sentry-native release direct from their github
"""

import requests
from zipfile import ZipFile

response = requests.get("https://api.github.com/repos/getsentry/sentry-native/releases/latest")
download_url = response.json()['assets'][0]['browser_download_url']
r = requests.get(download_url, allow_redirects=True)
open('sentry-native.zip', 'wb').write(r.content)

with ZipFile('sentry-native.zip', 'r') as zipObj:
    # Extract all the contents of zip file in current directory
    zipObj.extractall('sentry-native')