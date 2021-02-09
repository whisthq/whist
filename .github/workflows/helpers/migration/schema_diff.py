#!/usr/bin/env python
import os
import subprocess
from ignore import ignore_filter

CURRENT = os.environ["CURRENT"]
MERGING = os.environ["MERGING"]

# run migra to diff schema A and schema B
# decode result of subprocess
completed = subprocess.run(["migra", CURRENT, MERGING, "--unsafe"],
                           # check=True,
                           stdout=subprocess.PIPE)

diff = completed.stdout.decode("utf-8")

# filter out ignored schema patters
# includes version differences and random uid gens
# for ln in ignore_filter(diff.splitlines()):
#     print(ln)

for ln in diff.splitlines():
    print(ln)
