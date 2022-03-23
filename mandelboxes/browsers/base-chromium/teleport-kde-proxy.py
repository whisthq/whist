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
ERROR_LOG_PATH = "/home/whist/.teleport/logs/teleport-kde-proxy-err.out"


def handle_version_request():
    """
    Respond to a version query. Simply write out the expected kdialog
    version to stdout. Chrome uses this to check if kdialog is available.

    """

    print("kdialog 19.12.3")


def handle_open_single_file():
    """
    Handle a single file upload.

    Chrome runs each tab as a separate process, so just to be sure nothing strange happens we ensure
    that only one process can be in the upload file critical section at a time.

    We handle the file upload in these steps:
    - Remove everything from our temporary/staging uploads directory
    - Write the upload trigger file to notify the server to initiate a transfer
    - Wait for the server transfer to complete by looking for a confirmation/cancellation file
    - If confirmed - write the file name to stdout.

    The file path that is written to stdout will be used by chrome as the "selected" upload file.

    """

    with FileLock(FILE_UPLOAD_LOCK_PATH):
        # Nuke the uploads directory
        for file_name in os.listdir(FILE_UPLOAD_DIRECTORY):
            os.remove(os.path.join(FILE_UPLOAD_DIRECTORY, file_name))

        # Trigger protocol to initiate file transfer
        with open(FILE_UPLOAD_TRIGGER_PATH, "w") as file:
            file.write("upload-trigger")

        # Wait until upload confirmation or cancellation
        while not (
            os.path.exists(FILE_UPLOAD_CONFIRM_PATH) or os.path.exists(FILE_UPLOAD_CANCEL_PATH)
        ):
            time.sleep(0.1)

        if os.path.exists(FILE_UPLOAD_CANCEL_PATH):
            os.remove(FILE_UPLOAD_CANCEL_PATH)
        else:
            os.remove(FILE_UPLOAD_CONFIRM_PATH)
            # Should only be one file but get latest file in case
            file_glob = glob.glob(f"{FILE_UPLOAD_DIRECTORY}/*")
            target_file = max(file_glob, key=os.path.getmtime)
            # Output file path to stdout and chrome will handle the rest
            print(target_file)


def handle_kdialog():
    """Respond to chrome's intercepted kdialog call."""

    if "--version" in sys.argv:
        handle_version_request()
    elif "--getopenfilename" in sys.argv:
        # Currently we only do single file - multiple file will require more robust arg handling
        handle_open_single_file()


try:
    handle_kdialog()
except Exception as e:
    with open(ERROR_LOG_PATH, "w") as file:
        file.write(traceback.format_exc())
    raise e
