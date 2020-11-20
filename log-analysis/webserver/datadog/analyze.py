import boto3
import time
import json
import os

from functools import reduce

from dotenv import load_dotenv  # this might need to be pipped

from datetime import datetime
from datadog import initialize

from events import fetch, container_events_by_region, interval_avg, avg

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
WEEKLY_RECAP = "weekly-recap/"
DAILY_RECAP = "daily-recap/"

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

REGIONS = ["us-east-1", "ca-central-1"]


def initialize_datadog():
    s3 = boto3.resource("s3")

    # ok client isn't used but whatever
    file_objects = s3.Bucket(SECRET_BUCKET).objects.all()

    secrets_file = reduce(
        lambda acc, file: file.key if file.key == SECRET_FILE else acc,
        file_objects,
        None,
    )

    if secrets_file is None:
        raise RuntimeError(
            "Could not find datadog API secrets in the dev secrets bucket."
        )

    remote = secrets_file
    local = ".env"

    s3.Bucket(SECRET_BUCKET).download_file(remote, local)
    load_dotenv(local)

    options = {
        "api_key": os.getenv(DATADOG_API_KEY),
        "app_key": os.getenv(DATADOG_APP_KEY),
    }

    initialize(**options)


# below here are various helpers for what comes next because there is code repetition

# (boolean to avoid aliasing)
def _init_recap(textkey2label, textkey2label_val=False):
    recap = {tag: None for tag, _ in TAGS_TEXTKEYS}
    for tag in recap.keys():
        recap[tag] = {
            region: {
                textkey2label(textkey): {} if textkey2label_val else None
                for _, textkey in TAGS_TEXTKEYS
            }
            for region in REGIONS
        }
    return recap


def _for_tag_textkey_region_events(region2events, textkey2label, recap, f):
    for tag, textkey in TAGS_TEXTKEYS:
        for region in REGIONS:
            print(
                "analyzing tag {} with textkey {} with region {}".format(
                    tag, textkey, region
                )
            )
            events = region2events[region]
            events_with_tag = filter(lambda event: tag in event["tags"], events)

            f(
                tag=tag,
                textkey=textkey,
                region=region,
                label=textkey2label(textkey),
                events=events_with_tag,
                recap=recap,
            )


def _update_daily_recap_curry(now):
    def _update_daily_recap(**kwargs):
        tag, textkey, region = kwargs["tag"], kwargs["textkey"], kwargs["region"]
        label, events, recap = kwargs["label"], kwargs["events"], kwargs["recap"]

        recap[tag][region][label] = avg(
            events,
            textkey,
        )

    return _update_daily_recap


def _update_weekly_recap_curry(window, interval):
    def _update_weekly_recap(**kwargs):
        tag, textkey, region = kwargs["tag"], kwargs["textkey"], kwargs["region"]
        label, events, recap = kwargs["label"], kwargs["events"], kwargs["recap"]

        avgs = interval_avg(
            events,
            textkey,
            window=window,
            interval=interval,
        )

        # alternative to store on one file?
        for date, avg in avgs:
            try:
                # this is if it's a datetime object
                date_str = date.strftime("%Y%m%d")
            except:
                # probably unix time
                date_str = str(date)
            recap[tag][region][label][date_str] = avg

    return _update_weekly_recap


def _init_textkey2label(window):
    return (
        lambda textkey: "average_"
        + textkey
        + ("_by_day" if window == DAY else "_by_week" if window == WEEK else "")
    )


def _fetch_region2events(previous_time):
    all_events = fetch(previous_time, [tag for tag, _ in TAGS_TEXTKEYS])
    region2events = container_events_by_region(all_events, regions=REGIONS)
    return region2events

# here are the actual functions we use

# get the average metric of each region for each metric
def store_average_each_window(window=DAY):
    # THIS SHOULD BE RUN BY A DAILY CRON JOB

    # basically the same as below, we might want to combine the code or something
    # bruh code reuse plz
    client = boto3.client("s3")
    initialize_datadog()

    previous_time = time.time() - window
    region2events = _fetch_region2events(previous_time)

    textkey2label = _init_textkey2label(window)

    recap = _init_recap(textkey2label)
    print("Initialized datastructure.")

    now = datetime.now()
    _update_daily_recap = _update_daily_recap_curry(now)
    _for_tag_textkey_region_events(
        region2events, textkey2label, recap, _update_daily_recap
    )
    print("Finished analysis.")

    filename = DAILY_RECAP + now.strftime("%Y%m%d") + ".json"
    store_json = json.dumps(recap).encode("utf-8")
    store(filename, store_json, client=client)

    print("Stored file.")


# we can run this weekly to get the average usage per day... or whatever
# interval should work for day too but that would depend on the otherone not being messed up yikes
def store_averages_to_files_by_interval(window=DAY, interval=WEEK):
    # THIS SHOULD BE RUN BY A WEEKLY CRON JOB

    client = boto3.client("s3")
    initialize_datadog()

    textkey2label = _init_textkey2label(window)

    previous_time = time.time() - interval
    region2events = _fetch_region2events(previous_time)

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
    recap = _init_recap(textkey2label, textkey2label_val=True)
    print("Initialized datastructure.")

    _update_weekly_recap = _update_weekly_recap_curry(window, interval)
    _for_tag_textkey_region_events(
        region2events, textkey2label, recap, _update_weekly_recap
    )
    print("Finished analysis.")

    recap_json = json.dumps(recap).encode("utf-8")
    recap_filename = WEEKLY_RECAP + datetime.now().strftime("%Y%m%d") + ".json"
    store(recap_filename, recap_json, client=client)
    print("Stored file.")


# TODO also write to datadog event stream or something else
# todo clean this up depending on how we want to do it
def store(filename, binary_data, client=None):
    if not client:
        client = boto3.client("s3")

    client.put_object(Body=binary_data, Bucket=BUCKET, Key=filename)


if __name__ == "__main__":
    # TODO make an argparse in the future
    store_averages_to_files_by_interval()  # this is going to be run by the workflow
    store_average_each_window()
