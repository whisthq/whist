#!/bin/bash


echo "-----> Reading git branch and commit info from txt file "

echo $(ls)

exec 6< "git_info.txt"

read COMMIT <&6

export COMMIT=$COMMIT
export BRANCH=$HEROKU_BRANCH