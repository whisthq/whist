import time
import numpy as np

DAY = 60 * 60 * 24
WEEK = DAY * 7
DEFAULT_KEYS = ["life-time", "spinup-time", "shutdown-time"]


def last_week(tag="container-lifecycle"):
    # worried about conversions
    last_week = time.time() - WEEK

    return interval_avgs(list(fetch(last_week, tag_by=[tag])), DAY, keys=DEFAULT_KEYS)


# TODO
# change event formatting of text!
# very important!!! must be in format mentioned at bottom of code

# note that if keys is none it will assume that all the values
# are numerical!
def avgs(events, keys=None):
    avgs = {}
    totals = {}
    for event in events:
        for key in event.keys():
            if keys is None or key in keys:
                avgs[key] = avgs.get(key, 0) + event[key]
                totals[key] = totals.get(key, 0) + 1
    for key, val in avgs.items():
        avgs[key] = val / totals[key]
    return avgs


# return the avgs over each interval of events
# interval length should be in unix time length, so basically seconds I guess
# events must be a list type and not a generator
def interval_avgs(events, interval_length, keys=None):
    # events go from latest to oldest
    end_time = events[0]["date_happened"]
    averages = []
    window = []
    for event in events:
        if end_time - event["date_happened"] < interval_length:
            window.append(event)
        else:
            averages.append(avgs(window, keys=keys))

            window = []
            end_time = event["date_happened"]

    if len(window) > 0:
        averages.append(avgs(window, keys=keys))

    return averages


# generate all event objects ever since previous time
# previous_time must be in unix_time
def fetch(previous_time, tag_by, time_step=3600):
    time = time.time()

    while time > previous_time:
        # there may be one last window that is a bit smaller than the rest
        end_time, start_time = time, max(time - time_step, previous_time)
        yield from api.Event.query(
            start=start_time,
            end=end_time,
            # sources ?
            tags=tag_by,
            unaggregated=True,
        )["events"]

        time -= time_step


###### stuff here below basically formats events into dictionaries for easier work later ######

# the text/payload must be formatted like this:
# key:value\nkey:value\n etc
def parse_event_text(event_text):
    return {
        key: val for key, val in line.split(":", 1) for line in event_text.split("\n")
    }


def map_event(event):
    parsed = parse_event_text(event["text"])
    # it is necessary that we do not put some item called "title" or "tags" inside the event!
    parsed["title"] = event["title"]
    parsed["tags"] = event["tags"]
    return parsed  # TODO add timestamps


def map_events(events):
    yield from map(map_event, events)
