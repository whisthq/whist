#!/bin/bash


echo $(ls)
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

echo $PWD
echo $DIR
echo $(ls)

echo "-----> trying to get git info "

cd /app

echo $(ls)
exec 6< "git_info.txt"
read BRANCH <&6
read COMMIT <&6

cd ..
export BRANCH=$BRANCH
export COMMIT=$COMMIT

echo $BRANCH
echo $COMMIT