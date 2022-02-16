#!/usr/bin/env python3

import sys
import struct
import json
import os
import time

FILE_DOWNLOAD_TRIGGER_PATH = "/home/whist/.teleport/downloaded-file"
FILE_UPLOAD_TRIGGER_PATH = "/home/whist/.teleport/uploaded-file"


def send_message(message):
    encoded_content = json.dumps(message).encode("utf-8")
    # Write message size then the message itself
    sys.stdout.buffer.write(struct.pack("=I", len(encoded_content)))
    sys.stdout.buffer.write(encoded_content)
    sys.stdout.buffer.flush()


while True:
    bytes = sys.stdin.buffer.read(4)
    length = struct.unpack("I", bytes)[0]
    if length == -1:
        send_message({"action": "exit"})

    message = json.loads(sys.stdin.read(length))
    # Wait until we have consumed any existing trigger
    # TODO: Use inotify instead of polling. Probably not
    #       worth it unless and until we ditch python3.

    trigger_upload = message.get("fileUploadTrigger", False)
    trigger_download = message.get("downloadStatus", None)
    file_name = message.get("filename", "unnamed_file")

    if trigger_upload:
        trigger_file_path = FILE_UPLOAD_TRIGGER_PATH
        file_contents = "upload-trigger"
    elif trigger_download:
        trigger_file_path = FILE_DOWNLOAD_TRIGGER_PATH
        file_contents = file_name.rsplit("/")[-1]
    if trigger_upload or trigger_download:
        while os.path.exists(trigger_file_path):
            time.sleep(0.1)
        with open(trigger_file_path, "w") as file:
            file.write(file_contents)
