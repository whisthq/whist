#!/bin/bash

WHISTIUM_INSTANCE_IP=$1
WHISTIUM_INSTANCE_SSH_KEY=$2
WHISTIUM_INSTANCE_CHROMIUM_PATH=$3
MONOREPO_PATH=$(git rev-parse --show-toplevel)

# Apply patches
echo "Applying patches..."
PATCH_FOLDER=$MONOREPO_PATH/mandelboxes/browsers/whistium/patches
for filename in $(ls $PATCH_FOLDER) ; do
	echo $filename
	scp -i $WHISTIUM_INSTANCE_SSH_KEY $PATCH_FOLDER/$filename ubuntu@$WHISTIUM_INSTANCE_IP:/tmp/patch
	ssh -i $WHISTIUM_INSTANCE_SSH_KEY ubuntu@$WHISTIUM_INSTANCE_IP "cd $WHISTIUM_INSTANCE_CHROMIUM_PATH && git apply /tmp/patch"
done

echo "-------"

# Copy added files
echo "Adding files..."
ADDED_FOLDER=$MONOREPO_PATH/mandelboxes/browsers/whistium/custom_files
for filename in $(ls $ADDED_FOLDER) ; do
	echo $filename
	CHROMIUM_REPO_FILEPATH=$(echo $filename | tr "-" "/")
	CHROMIUM_REPO_DIRPATH="$(dirname $CHROMIUM_REPO_FILEPATH)"
	ssh -i $WHISTIUM_INSTANCE_SSH_KEY ubuntu@$WHISTIUM_INSTANCE_IP "mkdir -p $WHISTIUM_INSTANCE_CHROMIUM_PATH/$CHROMIUM_REPO_DIRPATH"
	scp -i $WHISTIUM_INSTANCE_SSH_KEY $ADDED_FOLDER/$filename ubuntu@$WHISTIUM_INSTANCE_IP:$WHISTIUM_INSTANCE_CHROMIUM_PATH/$CHROMIUM_REPO_FILEPATH
done