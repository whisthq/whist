#!/bin/bash

set -Eeuo pipefail

yarn tailwind
VERSION=$(git describe --abbrev=0) snowpack build
