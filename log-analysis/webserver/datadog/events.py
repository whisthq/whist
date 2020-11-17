import time
import numpy as np

from datadog import initialize, api

DAY = 60 * 60 * 24
WEEK = DAY * 7  # import this in the other one

# generate the averages of certain event values by textkey in events with a window and total interval
# generates datetime, avg val
# where datetime is of the last event in the events
def interval_avg(events, textkey, window, interval):
    # we return with timestamps by latest event in the interval
    window = []

    try:
        # if it's a list
        end_time = events[0]["date_happened"]
    except TypeError:
        # if it's a generator/iterator (for filter and map etc)
        # good for cpu utilization or something idk
        first_event = next(events)
        end_time = first_event["date_happend"]
        window.append(first_event)

    for event in events:
        if end_time - event["date_happened"] < interval_length:
            window.append(event)
        else:
            yield end_time, avg(window, textkey)

            window = []
            end_time = event["date_happened"]

    if len(window) > 0:
        yield end_time, avg(window, textkey)


def avg(events, textkey):
    return np.avg(
        list(map(lambda event: float(parse_event_text(event["text"])[textkey]), events))
    )


# generate all event objects ever since previous time
# previous_time must be in unix_time
def fetch(previous_time, tag_by, time_step=3600, format_version="1.0"):
    current_time = time.time()

    while current_time > previous_time:
        # there may be one last window that is a bit smaller than the rest
        end_time, start_time = current_time, max(
            current_time - time_step, previous_time
        )
        yield from filter(
            # important so that we only get the ones which have the right format for our fetch type
            lambda event: parse_event_text(event["text"])["format_version"]
            == format_version,
            api.Event.query(
                start=start_time,
                end=end_time,
                # sources ?
                tags=tag_by,
                unaggregated=True,
            )["events"],
        )
        current_time -= time_step


# the text/payload must be formatted like this:
# key:value\nkey:value\n etc
def parse_event_text(event_text):
    return {
        key: val for key, val in line.split(":", 1) for line in event_text.split("\n")
    }


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

            region2events[region].append(event)

        return region2events
    else:
        raise RuntimeError("Not supported with all regions.")


if __name__ == "__main__":
    # TODO this should not be committed
    options = {
        "api_key": None,  # TODO
        "app_key": None,
    }

    initialize(**options)

    for event in fetch(time.time() - WEEK, ["container-lifecycle"]):
        print(event, "\n")
