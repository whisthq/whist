#!/usr/bin/env python3

import glob
import os
import sys
import time
import traceback
from filelock import FileLock

FILE_UPLOAD_TRIGGER_PATH = "/home/whist/.teleport/uploaded-file"
FILE_UPLOAD_CONFIRM_PATH = "/home/whist/.teleport/uploaded-file-confirm"
FILE_UPLOAD_CANCEL_PATH = "/home/whist/.teleport/uploaded-file-cancel"
FILE_UPLOAD_LOCK_PATH = "/home/whist/.teleport/uploaded-file-lock"
FILE_UPLOAD_DIRECTORY = "/home/whist/.teleport/uploads"
LOG_PATH = "/home/whist/.teleport/logs/teleport-kde-proxy-err.out"


def handle_version_request():
    print("kdialog 19.12.3")


def handle_open_single_file():
    with FileLock(FILE_UPLOAD_LOCK_PATH):
        # Nuke the uploads directory
        for file_name in os.listdir(FILE_UPLOAD_DIRECTORY):
            os.remove(os.path.join(FILE_UPLOAD_DIRECTORY, file_name))

        # Trigger protocol to initiate file transfer
        with open(FILE_UPLOAD_TRIGGER_PATH, "w") as file:
            file.write("upload-trigger")

        # Wait until upload confirmation or cancellation
        while not (
            os.path.exists(FILE_UPLOAD_CONFIRM_PATH) and os.path.exists(FILE_UPLOAD_CONFIRM_PATH)
        ):
            time.sleep(0.1)
        os.remove(FILE_UPLOAD_CONFIRM_PATH)

        # Should only be one file but get latest file in case
        file_glob = glob.glob(f"{FILE_UPLOAD_DIRECTORY}/*")
        target_file = max(file_glob, key=os.path.getmtime)

        # Output file path to stdout and chrome will handle the rest
        print(target_file)


def handle_kdialog():
    if "--version" in sys.argv:
        handle_version_request()
    elif "--getopenfilename" in sys.argv:
        handle_open_single_file()


try:
    handle_kdialog()
except Exception as e:
    with open(LOG_PATH, "w") as file:
        file.write(traceback.format_exc())
    raise e
