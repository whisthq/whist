#!/bin/bash

CHROMIUM_PATH=$1
MONOREPO_PATH=$(git rev-parse --show-toplevel)

# Apply patches
echo "Applying patches..."
PATCH_FOLDER=$MONOREPO_PATH/mandelboxes/browsers/whistium/patches
for filename in $(ls $PATCH_FOLDER) ; do
	echo $filename
	cd $CHROMIUM_PATH && git apply $PATCH_FOLDER/$filename
done

echo "-------"

# Copy added files
echo "Adding files..."
ADDED_FOLDER=$MONOREPO_PATH/mandelboxes/browsers/whistium/custom_files
for filename in $(ls $ADDED_FOLDER) ; do
	echo $filename
	CHROMIUM_REPO_FILEPATH=$(echo $filename | tr "-" "/")
	cp $ADDED_FOLDER/$filename $CHROMIUM_PATH/$CHROMIUM_REPO_FILEPATH
done