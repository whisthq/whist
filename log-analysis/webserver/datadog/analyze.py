import boto3
import time
import json  # so human readable
import os

from functools import reduce

from dotenv import load_dotenv  # this might need to be pipped

from datetime import datetime
from datadog import initialize

from events import fetch, container_events_by_region, interval_avg

## TODO
# we will need to fetch

# 3. DATADOG_API_KEY
# 4. DATADOG_APP_KEY
# 5... possibly also the region
from os import getenv

SECRET_BUCKET = "fractal-dev-secrets"
SECRET_FILE = "datadog-api"

DATADOG_API_KEY = "DATADOG_API_KEY"
DATADOG_APP_KEY = "DATADOG_APP_KEY"

BUCKET = "fractal-webserver-logs"
FILEPATH = "{tag}/{region}/"
FILENAME = "{date}-{label}"
RECAP = "recap/"
# we store everything in
# tag/region/date-label
#   for example:
#       container-creation/us-east-1/20201117-average-container-time-past-day
# could correspond to the file of averages of container times for the past day
# or something like that. It is a bit inefficient if we store in various different files,
# so if we don't have a lot of different metrics I might just put them together in one bigger
# file

# temporal constants
DAY = 60 * 60 * 24
WEEK = DAY * 7

# what we are looking for:
# 1. average time to container startup per region daily
# 2. average number of containers/container lifetime per region per day

# these are the tags that we want to identify different averages by as requested above
# we can add more later; they are copied from app/helpers/utils/datadog.event_tags.py in webserver
# in the future we may want one easy place to get these constants for both... hmmm...
CONTAINER_CREATION = "container-creation"
CONTAINER_LIFECYCLE = "container-lifecycle"

SPINUP_TIME = "spinup_time"
LIFETIME = "lifetime"

TAGS_TEXTKEYS = [
    (CONTAINER_CREATION, SPINUP_TIME),
    (CONTAINER_LIFECYCLE, LIFETIME),
]

REGIONS = [
    "us-east-1",
]


def initialize_datadog():
    s3 = boto3.resource("s3")

    # ok client isn't used but whatever
    file_objects = s3.Bucket(SECRET_BUCKET).objects.all()

    secrets_file = reduce(
        lambda acc, file: file.key if file.key == SECRET_FILE else acc, None
    )

    if secrets_file is None:
        raise RuntimeError(
            "Could not find datadog API secrets in the dev secrets bucket."
        )

    remote = secrets_file.key
    local = ".env"

    s3.Bucket(SECRET_BUCKET).download_file(remote, local)
    load_dotenv(local)

    options = {
        "api_key": os.getenv(DATADOG_API_KEY),
        "app_key": os.getenv(DATADOG_APP_KEY),
    }

    initialize(**options)


# we can run this weekly to get the average usage per day... or whatever
# interval should work for day too but that would depend on the otherone not being messed up yikes
def store_averages_to_files(window=DAY, interval=WEEK):
    # this should be allowed via lambda or whatever
    client = boto3.client("s3")
    initialize_datadog(client)

    # this will be helpful if we are running a lambda because
    # we want to avoid it going over 15 minutes since otherwise
    # it might shutdown during work which is worrisome
    lambda_time_start = time.time()

    textkey2label = (
        lambda textkey: "average_"
        + textkey
        + ("_by_day" if window == DAY else "_by_week" if window == WEEK else "")
    )

    previous_time = time.time() - interval
    all_events = fetch(previous_time, [tag for tag, _ in TAGS_TEXTKEYS])
    region2events = container_events_by_region(all_events, regions=REGIONS)

    # json object to be stored in the recap
    # json has format like the files
    # {
    #    tag: {
    #        region: {
    #            label : {
    #                [str] : [int] (datetime strftime : integer average value)
    #            },
    #        }
    #    }
    # }
    recap = {tag: None for tag, _ in TAGS_TEXTKEYS}
    for tag in recap.keys():
        recap[tag] = {
            region: {textkey2label(textkey): {} for _, textkey in TAGS_TEXTKEYS}
            for region in REGIONS
        }

    # go and find the averages, add to the recap, and then also add to each individual file
    # we probably won't actually save anything but the recap until we have more detailed logs, not sure
    for tag, textkey in TAGS_TEXTKEYS:
        for region in REGIONS:
            events = region2events[region]

            # only supports day now
            label = textkey2label(textkey)

            filepath = FILEPATH.format(tag=tag, region=region)

            avgs = interval_avg(
                filter(lambda event: tag in event["tags"], events),
                textkey,
                window=window,
                interval=interval,
            )

            # alternative to store on one file?
            for date, avg in avgs:
                date_str = date.strftime("%Y%m%d")
                recap[tag][region][label][date_str] = avg

                # won't be using this for now
                # filename = filepath + FILENAME.format(date=date_str, label=label)
                # store_json = json.dumps({label: avg}).encode("utf-8")
                # store(filename, store_json, client=client)

    # this is in seconds
    recap["lambda_time"] = round(time.time() - lambda_time_start)  # in seconds

    recap_json = json.dumps(recap).encode("utf-8")
    recap_filename = RECAP + datetime.now().strftime("%Y%m%d")
    store(recap_filename, recap_json, client=client)


# todo clean this up depending on how we want to do it
def store(filename, binary_data, client=None):
    if not client:
        client = boto3.client("s3")

    client.put_object(Body=binary_data, Bucket=BUCKET, Key=filename)


if __name__ == "__main__":
    store_averages_to_files()
