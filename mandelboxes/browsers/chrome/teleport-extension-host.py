#!/usr/bin/env python3

import sys
import struct
import json
import os
import time

FILE_DOWNLOAD_TRIGGER_PATH = "/home/whist/.teleport/downloaded-file"

while True:
    bytes = sys.stdin.buffer.read(4)
    length = struct.unpack("I", bytes)[0]
    message = json.loads(sys.stdin.read(length))
    # Wait until we have consumed any existing trigger
    # TODO: Use inotify instead of polling. Probably not
    #       worth it unless and until we ditch python3.
    while os.path.exists(FILE_DOWNLOAD_TRIGGER_PATH):
        time.sleep(0.1)
    with open(FILE_DOWNLOAD_TRIGGER_PATH, "w") as file:
        file.write(message["filename"].rsplit("/")[-1])
