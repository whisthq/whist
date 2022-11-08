#!/bin/bash

CHROMIUM_PATH=$1
MONOREPO_PATH=$(git rev-parse --show-toplevel)

# Create patches for modified files
echo "Creating patch files..."
PATCH_FOLDER=$MONOREPO_PATH/mandelboxes/browsers/whistium/patches
if [ ! -d $PATCH_FOLDER ]; then
	mkdir -p $PATCH_FOLDER
fi
for filepath in $(cd $CHROMIUM_PATH && git diff --name-only) ; do
	PATCH_FILENAME=$(echo $filepath | tr "/" "-")
	PATCH_FILENAME=${PATCH_FILENAME%.*}.patch
	echo $PATCH_FILENAME
	cd $CHROMIUM_PATH && git diff $filepath > $PATCH_FOLDER/$PATCH_FILENAME
done

echo "-------"

# Copy added files
echo "Copying added files..."
ADDED_FOLDER=$MONOREPO_PATH/mandelboxes/browsers/whistium/custom_files
if [ ! -d $ADDED_FOLDER ]; then
	mkdir -p $ADDED_FOLDER
fi
for filepath in $(cd $CHROMIUM_PATH && git status --porcelain | awk 'match($1, "\\?\\?"){print $2}') ; do
	if [ -d $CHROMIUM_PATH/$filepath ]; then
		continue
	fi
	ADDED_FILENAME=$(echo $filepath | tr "/" "-")
	echo $ADDED_FILENAME
	cp $CHROMIUM_PATH/$filepath $ADDED_FOLDER/$ADDED_FILENAME
done