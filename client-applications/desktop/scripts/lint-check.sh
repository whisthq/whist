#!/bin/bash

set -Eeuo pipefail

yarn eslint "./src/**/*.{js,jsx,ts,tsx}"
