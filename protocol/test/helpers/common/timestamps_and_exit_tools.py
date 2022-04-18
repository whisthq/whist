#!/usr/bin/env python3

import time
import datetime
import sys
import os

# Add the current directory to the path no matter where this is called from
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "."))

# Acknowledged to https://stackoverflow.com/questions/287871/how-do-i-print-colored-text-to-the-terminal-in-python
class PrintFormats:
    blue = "\033[94m"
    cyan = "\033[96m"
    green = "\033[92m"
    yellow = "\033[93m"
    red = "\033[91m"
    bold = "\033[1m"
    underline = "\033[4m"
    end_formatting = "\033[0m"


def printblue(text):
    print(f"{PrintFormats.blue}{text}{PrintFormats.end_formatting}")


def printcyan(text):
    print(f"{PrintFormats.cyan}{text}{PrintFormats.end_formatting}")


def printgreen(text):
    print(f"{PrintFormats.green}{text}{PrintFormats.end_formatting}")


def printyellow(text):
    print(f"{PrintFormats.yellow}{text}{PrintFormats.end_formatting}")


def printred(text):
    print(f"{PrintFormats.red}{text}{PrintFormats.end_formatting}")


def printbold(text):
    print(f"{PrintFormats.bold}{text}{PrintFormats.end_formatting}")


def printunderline(text):
    print(f"{PrintFormats.underline}{text}{PrintFormats.end_formatting}")


class TimeStamps:
    def __init__(self):
        self.start_time = time.time()
        self.events = []
        self.max_event_name_len = 0
        self.most_time_consuming_event = datetime.timedelta(seconds=0)

    def get_time_elapsed(self, index):
        if index < 0 or index >= len(self.events):
            return -1
        return self.events[index][1] - self.events[max(0, index - 1)][1]

    def add_event(self, event_name):
        timestamp = time.time()
        self.max_event_name_len = max(self.max_event_name_len, len(event_name))

        self.events.append((event_name, timestamp))
        event_index = len(self.events)
        time_elapsed = datetime.timedelta(seconds=self.get_time_elapsed(event_index - 1))
        self.most_time_consuming_event = max(self.most_time_consuming_event, time_elapsed)
        print(f"{event_name} took {str(time_elapsed)}")

    def printtimestamps(self):
        print()
        print("**** E2E time breakdown ****")
        for i, event in enumerate(self.events):
            event_name = event[0]
            time_elapsed = datetime.timedelta(seconds=self.get_time_elapsed(i))
            # Add padding for nicer formatting
            padding_len = self.max_event_name_len - len(event_name)
            padding = " " * padding_len if padding_len > 0 else ""
            if time_elapsed == self.most_time_consuming_event:
                printbold(f"{i+1}. {event_name}{padding}:\t{str(time_elapsed)}")
            else:
                print(f"{i+1}. {event_name}{padding}:\t{str(time_elapsed)}")
        print("############################")
        print()


def exit_with_error(error_message, timestamps=None):
    if error_message is not None:
        printred(error_message)
    if timestamps is not None:
        timestamps.printtimestamps()
    sys.exit(-1)
