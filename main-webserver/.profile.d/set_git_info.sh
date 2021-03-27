#!/bin/bash

set -Eeuo pipefail

echo "-----> Reading git branch and commit info from txt file "

IN_CI=${CI:=false} # default: false
if [ $IN_CI == false ]; then


  echo $HEROKU_BRANCH

  export BRANCH=$HEROKU_BRANCH
fi

