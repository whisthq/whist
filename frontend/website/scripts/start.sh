#!/bin/bash

set -Eeuo pipefail

yarn tailwind && yarn pull:assets && snowpack dev
