#!/bin/bash


echo "-----> Reading git branch and commit info from txt file "

echo $(ls)

# cd bin 

# echo $(ls)
# exec 6< "bin/git_info.txt"
# read BRANCH <&6
# read COMMIT <&6
BRANCH=$(git branch --show-current)
COMMIT=$(git rev-parse --short HEAD)


export BRANCH=$BRANCH
export COMMIT=$COMMIT

echo $BRANCH
echo $COMMIT