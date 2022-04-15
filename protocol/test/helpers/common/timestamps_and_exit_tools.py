#!/usr/bin/env python3

import time
import datetime
import sys
import os

# Add the current directory to the path no matter where this is called from
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "."))


class TimeStamps:
    def __init__(self):
        self.start_time = time.time()
        self.events = []
        self.max_event_name_len = 0

    def get_time_elapsed(self, index):
        if index < 0 or index >= len(self.events):
            return -1
        return self.events[index][1] - self.events[max(0, index - 1)][1]

    def add_event(self, event_name):
        timestamp = time.time()
        self.events.append((event_name, timestamp))
        self.max_event_name_len = max(self.max_event_name_len, len(event_name))
        time_elapsed = str(datetime.timedelta(seconds=self.get_time_elapsed(len(self.events) - 1)))
        print(f"{event_name} took {time_elapsed}")

    def print_timestamps(self):
        print()
        print("**** E2E time breakdown ****")
        for i, event in enumerate(self.events):
            event_name = event[0]
            time_elapsed = str(datetime.timedelta(seconds=self.get_time_elapsed(i)))
            # Add padding for nicer formatting
            padding_len = self.max_event_name_len - len(event_name)
            padding = " " * padding_len if padding_len > 0 else ""
            print(f"{i+1}. {event_name}{padding}:\t{time_elapsed}")
        print("############################")
        print()


def exit_with_error(error_message, timestamps=None):
    if error_message is not None:
        print(error_message)
    if timestamps is not None:
        timestamps.print_timestamps()
    sys.exit(-1)
