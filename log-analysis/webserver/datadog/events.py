import time
import numpy as np

from datetime import datetime

from datadog import api

DAY = 60 * 60 * 24
WEEK = DAY * 7  # import this in the other one

# generate the averages of certain event values by textkey in events with a window and total interval
# generates datetime, avg val
# where datetime is of the last event in the events
def interval_avg(events, textkey, window, interval):
    # we return with timestamps by latest event in the interval
    window = []
    try:
        if not type(events) == list:
            events = list(events)

        end_time = events[0]["date_happened"]

        for event in events:
            if end_time - event["date_happened"] < interval:
                window.append(event)
            else:
                yield end_time, avg(window, textkey)

                window = []
                end_time = event["date_happened"]

        if len(window) > 0:
            yield end_time, avg(window, textkey)
    # these two are for when there is nothing basically
    except IndexError:
        # this happens when [0] -> len == 0 so error
        # and we in that case want to yield nothing (i.e. it's empty, the data
        # structures using this function already have that assumped i.e. as if there
        # was no data, it's only when there is data that they change)
        pass


# this is kind of a hack
def event_val_num(event, textkey):
    textvalue = parse_event_text(event["text"])[textkey]
    try:
        return float(textvalue)
    except ValueError:
        # in our case should happen in the case of a datetime object (really timedelta)3
        # because we can only strptime with datetime we do this (get a timedelta td):
        td = datetime.strptime("0:01:13.964650", "%H:%M:%S.%f")
        td = td - datetime(
            year=td.year, month=td.month, day=td.day
        )  # normally will be 1900, 1, 1
        return td.total_seconds()


def avg(events, textkey):
    return np.average(list(map(lambda event: event_val_num(event, textkey), events)))


def right_format(event, format_version):
    try:
        text = event["text"]
        parsed_format = parse_event_text(text)["format_version"]
        # print("parseable!")
        return parsed_format == format_version
    except Exception as e:
        # print(f"exception ->> {str(e)} <<- with an event")
        # this should happen for really old events basically
        return False


# generate all event objects ever since previous time
# previous_time must be in unix_time
def fetch(previous_time, tag_by, time_step=3600, format_version="1.0"):
    current_time = time.time()

    while current_time > previous_time:
        # there may be one last window that is a bit smaller than the rest
        end_time, start_time = current_time, max(
            current_time - time_step, previous_time
        )

        # unfortunately datadog docs are a bit misleading
        # it looks like the list of tags is an AND and not an OR
        # as it is in the event stream
        for tag in tag_by:
            events = api.Event.query(
                start=start_time,
                end=end_time,
                # sources ?
                tags=[tag],  # tag_by,
                unaggregated=True,
            )

            if type(events) == dict:
                events = events["events"]
            if type(events) == tuple:
                events = events[0] if len(events) > 0 else []

            # unfortunately we might need to sort again
            # because we are doing this by tags
            # it might not be necessary since only the already-sorted subsequences
            # are observed
            yield from sorted(
                filter(
                    # important so that we only get the ones which have the right format for our fetch type
                    lambda event: right_format(event, format_version),
                    events,
                ),
                key=lambda event: event["date_happened"],
                reverse=True,
            )

        current_time -= time_step


# the text/payload must be formatted like this:
# key:value\nkey:value\n etc
def parse_event_text(event_text):
    parsed = {}
    # python can't do nested for loops in dict comprehension?!
    for line in event_text.split("\n"):
        key, val = line.split(":", 1)
        parsed[key] = val
    return parsed


def container_name2region(container_name):
    # names have format like "arn:aws:ecs:us-east-1:etcetc"
    # which is like "arn:aws:ecs:<region>:randomgibberish"
    _, _, _, region, _ = container_name.split(":", 4)
    return region


def container_events_by_region(all_events, regions=None):
    if regions:  # we do this at the top level to avoid lots of ifs in the loop
        region2events = {region: [] for region in regions}
        for event in all_events:
            text_keyvals = parse_event_text(event["text"])
            container_name = text_keyvals["container_name"]
            region = container_name2region(container_name)

            if region in region2events:
                region2events[region].append(event)
            else:
                print(f"\tregion {region} was found, but not in desired regions")

        return region2events
    else:
        raise RuntimeError("Not supported with all regions.")


# take a recap object and log metrics
# recap objects are
# {
#    tag: {
#        region: {
#            datetime : [int] representing the metric by label for that day in region by tag
#        }
#    }
# }
# datadog must be initialized for you to use this
def datadog_log_metrics_daily(recap):
    now = round(time.time())

    # we'll be sending daily
    # you can also send in batches
    # and that should be easy to implement for the
    # version of this for weeks (i.e. intervals)
    # by just getting each object with the datetime and then sending
    # the list of items() inside the region_obj[label] which is a dict
    # of timestamp to val
    for tag, tag_obj in recap.items():
        for region, region_obj in tag_obj.items():
            for label, metric_val in region_obj.items():
                metric = tag + "." + region + "." + label
                print(metric, metric_val)
                api.Metric.send(
                    metric=metric, points=(now, 0 if metric_val is None else metric_val)
                )
