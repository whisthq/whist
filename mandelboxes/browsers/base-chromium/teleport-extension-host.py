#!/usr/bin/env python3

import sys
import struct
import json
import os
import time

FILE_DOWNLOAD_TRIGGER_PATH = "/home/whist/.teleport/downloaded-file"
POINTER_LOCK_TRIGGER_PATH = "/home/whist/.teleport/pointer-lock-update"
DID_UPDATE_EXTENSION_TRIGGER_PATH = "/home/whist/.teleport/did-update-extension"

def send_message(message):
    """
    Send message to connected chrome extension by writing to stdout buffer

    Args:
        message: Dict to encode as a json stream and send

    """
    encoded_content = json.dumps(message).encode("utf-8")
    # Write message size then the message itself
    sys.stdout.buffer.write(struct.pack("=I", len(encoded_content)))
    sys.stdout.buffer.write(encoded_content)
    sys.stdout.buffer.flush()


def read_message():
    """
    Get message from connected chrome extension by reading from stdin buffer.
    If the message value is -1, then signal to close the connection. This function
    will block until a message is received.

    Returns:
        message: Dict of the message received, or None if the native host is closing.

    """
    # Blocks until message is received
    bytes = sys.stdin.buffer.read(4)
    if len(bytes) != 4:
        # The connection has been closed by the extension reloading or similar.
        return None
    message_size = struct.unpack("=I", bytes)[0]
    if message_size == -1:
        # The browser is exiting, so tell the extension to gracefully disconnect this
        # native host. (Note: This -1 value is undocumented but works.)
        return None
    return json.loads(sys.stdin.buffer.read(message_size).decode("utf-8"))


def write_trigger(trigger_path, payload, poll_interval=0.1, sequential=True):
    """
    Trigger the protocol to be run by writing to the trigger file.

    Args:
        trigger_path: Path to the trigger file
        payload: Payload to write to the trigger file
        poll_interval: Interval to poll the trigger file if sequential is True
        sequential: If False, then the trigger file will be overwritten if it already exists.
                    Otherwise, we will wait for the trigger file to be removed before writing,
                    so that the protocol can handle trigger events sequentially.
    """

    # Wait until any existing download trigger is handled
    # TODO: Use inotify instead of polling. Probably not
    #       worth it unless and until we ditch python3.
    if sequential:
        while os.path.exists(trigger_path):
            time.sleep(poll_interval)

    # Write the trigger file
    with open(trigger_path, "w") as f:
        f.write(payload)


def main():
    # If the extension hasn't tried updating yet, then trigger an update and keep
    # track of the fact that we've tried by writing to the trigger file. Otherwise,
    # just send a message to the extension that we're ready to go.
    if not os.path.exists(DID_UPDATE_EXTENSION_TRIGGER_PATH):
        write_trigger(DID_UPDATE_EXTENSION_TRIGGER_PATH, "", sequential=False)
        send_message({"type": "NATIVE_HOST_UPDATE", "value": True})
    else:
        send_message({"type": "NATIVE_HOST_UPDATE", "value": False})

    longitude = os.getenv("LONGITUDE")
    latitude = os.getenv("LATITUDE")

    print("The longitude is", longitude)
    print("The latitude is", latitude)

    if (not longitude == "") and (not latitude == ""):
        send_message({"type": "GEOLOCATION", "value": {
            longitude: longitude,
            latitude: latitude
        }})

    # Enter main message handling loop
    while True:
        message = read_message()
        if message is None:
            # The extension is shutting down, so we can exit the main loop.
            break

        # TODO: Switch to Python 3.10 and use match-case for this
        if message["type"] == "DOWNLOAD_COMPLETE":
            absolute_filename = message["value"]
            relative_filename = absolute_filename.rsplit("/")[-1]
            write_trigger(FILE_DOWNLOAD_TRIGGER_PATH, relative_filename)
        elif message["type"] == "POINTER_LOCK":
            # Pointer lock events technically don't need to be sequential=True since
            # we only care about latest state, but we do it for now to avoid a potential
            # protocol race condition between reading and deleting the trigger file.
            write_trigger(POINTER_LOCK_TRIGGER_PATH, "1" if message["value"] else "0")


main()
