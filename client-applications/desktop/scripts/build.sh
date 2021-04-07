#!/bin/bash

set -Eeuo pipefail

yarn tailwind
snowpack build
