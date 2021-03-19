#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

echo $DIR
echo $(ls)

echo "-----> fuck you leonard"



exec 6< "${GIT_DIR}/git_info.txt"
read BRANCH <&6
read COMMIT <&6

export BRANCH=$BRANCH
export COMMIT=$COMMIT

echo $BRANCH
echo $COMMIT