#!/bin/bash

set -Eeuo pipefail

npx tailwindcss-cli@latest build -o public/css/tailwind.css
