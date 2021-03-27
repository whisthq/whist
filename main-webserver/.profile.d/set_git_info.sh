#!/bin/bash

set -Eeuo pipefail

echo "-----> Reading git branch and commit info from txt file "

IN_CI=${CI:=false} # default: false
if [ $IN_CI == false ]; then
  export BRANCH=$HEROKU_BRANCH
fi

