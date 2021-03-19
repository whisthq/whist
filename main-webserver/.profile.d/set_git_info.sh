#!/bin/bash


echo "-----> Reading git branch and commit info from txt file "

echo $(ls)
exec 6< "git_info.txt"
read BRANCH <&6
read COMMIT <&6

export BRANCH=$BRANCH
export COMMIT=$COMMIT

echo $BRANCH
echo $COMMIT