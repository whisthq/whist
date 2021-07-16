#!/bin/bash

set -Eeuo pipefail

yarn eslint --max-warnings 0 --fix "src/**/*.ts*"
