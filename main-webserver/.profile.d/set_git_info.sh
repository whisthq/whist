#!/bin/bash

set -Eeuo pipefail

echo "-----> Reading git branch and commit info from txt file "

IN_CI=${CI:=false} # default: false
if [ $IN_CI == false ]; then
  exec 6< "git_info.txt"

  read COMMIT <&6

  echo $COMMIT
  echo $HEROKU_BRANCH

  export COMMIT=$COMMIT
  export BRANCH=$HEROKU_BRANCH
fi

