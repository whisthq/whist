#!/bin/bash

set -Eeuo pipefail

yarn tailwind
yarn assets:pull
snowpack build

# We need to let the React app handle all routing login.
# Without this, directly navigating to whist.com/about fails.
echo '/* /index.html 200' > build/_redirects
