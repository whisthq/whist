#!/bin/bash

set -Eeuo pipefail

yarn tailwind
yarn assets:pull
snowpack dev
