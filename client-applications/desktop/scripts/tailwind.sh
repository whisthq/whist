#!/bin/bash

set -Eeuo pipefail

npx tailwindcss build -o public/css/tailwind.css
