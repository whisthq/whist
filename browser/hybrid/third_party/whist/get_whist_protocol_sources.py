#!/usr/bin/env python3
from pathlib import Path
import paths

# This file is used to determine sources for the Whist Client Library build
# step in BUILD.gn. It is called via exec_script, and the lines printed to stdout
# are used to generate the sources list in the GN file. This allows the Chromium
# build to re-run the protocol build step in response to changes in these listed
# source files.

# TODO: Maybe the below should be more granular, and/or respond to .gitignored
# files. Else, maybe we can use CMake functionality to generate a file list.

files = []
for ext in ["c", "cpp", "m", "mm", "h", "cu", "png", "txt"]:
    for file in Path(paths.PROTOCOL_ROOT).glob(f"**/*.{ext}"):
        print(f"//{str(file.relative_to(paths.CHROMIUM_SRC_ROOT))}")
