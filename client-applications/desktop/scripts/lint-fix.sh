#!/bin/bash

set -Eeuo pipefail


yarn eslint --max-warnings 0 --fix "./src/**/*.{js,jsx,ts,tsx}"
