#!/bin/bash

set -Eeuo pipefail

echo "-----> Reading git branch and commit info from txt file "

echo $(ls)
IN_CI=${CI:=false} # default: false
if [ $IN_CI == false ]; then
  exec 6< "git_info.txt"

  read COMMIT <&6

  export COMMIT=$COMMIT
  export BRANCH=$HEROKU_BRANCH
fi

