#!/bin/bash
set -euo pipefail

# To pass a value to the GitHub Actions outputs, you must format it like the
# string below, where name= is the name of the output as it appears in
# action.yml.

# Here, we reference a STATE_CONFIG value, which should contain the result
# from entrypoint.sh. You must prefix variables with STATE_ to pass them
# between the pre-entrypoint.sh, entrypoint.sh, and post-entrypoint.sh
# processes.

echo "::set-output name=config::$(echo $STATE_CONFIG)"
