#!/usr/bin/env python3

import glob
import os
import sys
import time
import traceback

FILE_UPLOAD_TRIGGER_PATH = "/home/whist/.teleport/uploaded-file"
FILE_UPLOAD_CONFIRM_PATH = "/home/whist/.teleport/uploaded-file-confirm"
FILE_UPLOAD_DIRECTORY = "/home/whist/.teleport/uploads"
LOG_PATH = "/home/whist/.teleport/logs/teleport-kde-proxy-err.out"

def handle_version_request():
    print("kdialog 19.12.3")

def handle_open_single_file():
    # Nuke the uploads directory
    for file_name in os.listdir(FILE_UPLOAD_DIRECTORY):
        os.remove(os.path.join(FILE_UPLOAD_DIRECTORY, file_name))

    # Trigger protocol to initiate file transfer
    with open(FILE_UPLOAD_TRIGGER_PATH, "w") as file:
        file.write("upload-trigger")

    # Wait until file hits uploads directory
    while(len(os.listdir(FILE_UPLOAD_DIRECTORY)) == 0):
        time.sleep(0.1)

    # Should only be one file
    file_glob = glob.glob(f"{FILE_UPLOAD_DIRECTORY}/*")
    target_file = max(file_glob, key=os.path.getmtime)

    # FIXME: (HACK) Implement WCCLIENT message to deterministically wait for file uploads/cancellations
    # or figure out a better way for confirm triggers (or maybe use fuse?)
    # Wait for confirm trigger now (another file will be written on completion) and then that'll
    # be the newest one (yes this is borked. will fix tmr)
    # This is a hack - race condition paradise with large vs small files in succession
    latest_upload = target_file
    prev_upload = target_file
    while(prev_upload == latest_upload):
        prev_upload = latest_upload
        file_glob = glob.glob(f"{FILE_UPLOAD_DIRECTORY}/*")
        latest_upload = max(file_glob, key=os.path.getmtime)
    
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
