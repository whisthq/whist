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
    """
    The TimeStamps class provides functions to easily track the time elapsed in each section of the E2E code
    and print a summary table at the end.

    Sample Usage:

        timestamps = TimeStamps()
        < instruction block A >
        timestamps.add_event("block A")
        < instruction block B >
        timestamps.add_event("block B")

        if something_bad_happened:
            exit_with_error("Oh no, something terrible just happened. Exiting now!", timestamps=timestamps)
        else:
            timestamps.print_timestamps()
    """

    def __init__(self):
        self.start_time = time.time()
        self.events = []
        self.max_event_name_len = 0
        self.most_time_consuming_event = datetime.timedelta(seconds=0)

    def __get_time_elapsed(self, index):
        # Returns the time elapsed in the block of code with a given index in the list of events
        if index < 0 or index >= len(self.events):
            return -1
        return self.events[index][1] - self.events[max(0, index - 1)][1]

    def add_event(self, event_name):
        """
        Record the time elapsed in the block of code between the previous call to this function
        (or the initialization of this class if this is the first call) and the present call
        to this function.

        Args:
            event_name (str): A name to assign to the block of code that we're seeking to time

        Returns:
            None
        """
        timestamp = time.time()
        self.max_event_name_len = max(self.max_event_name_len, len(event_name))

        self.events.append((event_name, timestamp))
        event_index = len(self.events)
        time_elapsed = datetime.timedelta(seconds=self.__get_time_elapsed(event_index - 1))
        self.most_time_consuming_event = max(self.most_time_consuming_event, time_elapsed)
        print(f"{event_name} took {str(time_elapsed)}")

    def print_timestamps(self):
        """
        Print a summary table with the time elapsed in each block of the code

        Args:
            None

        Returns:
            None
        """
        print()
        print("**** E2E time breakdown ****")
        for i, event in enumerate(self.events):
            event_name = event[0]
            time_elapsed = datetime.timedelta(seconds=self.__get_time_elapsed(i))
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
    """
    Exit the process calling this function with error code -1. Optionally, also print an error message in red
    and print a summary table with the time elapsed in each part of the E2E experiment.

    Args:
        error_message (str):  The error message to print in red, or None if you don't want to print any message
        timestamps (TimeStamps):    The object containing the data to print the summary table with the time elapsed
                                    in each part of the E2E experiment, or None if you don't want to print the
                                    summary table

    Returns:
        None and exit with exitcode -1
    """
    if error_message is not None:
        printred(error_message)
    if timestamps is not None:
        timestamps.print_timestamps()

    # In case of errors, the instances used in the test were likely not shut down properly, so we need to do so manually.
    print("If running locally, don't forget to remove leftover instances with the command below:")
    printblue("python3 -m helpers.aws.remove_leftover_instances")

    sys.exit(-1)
