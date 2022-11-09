#!/bin/bash

WHISTIUM_INSTANCE_IP=$1
WHISTIUM_INSTANCE_SSH_KEY=$2
WHISTIUM_INSTANCE_CHROMIUM_PATH=$3
MONOREPO_PATH=$(git rev-parse --show-toplevel)

# Create patches for modified files
echo "Creating patch files..."
PATCH_FOLDER=$MONOREPO_PATH/mandelboxes/browsers/whistium/patches
if [ ! -d $PATCH_FOLDER ]; then
	mkdir -p $PATCH_FOLDER
fi
for filepath in $(ssh -i $WHISTIUM_INSTANCE_SSH_KEY ubuntu@$WHISTIUM_INSTANCE_IP "cd $WHISTIUM_INSTANCE_CHROMIUM_PATH && git diff --name-only") ; do
	PATCH_FILENAME=$(echo $filepath | tr "/" "-")
	PATCH_FILENAME=${PATCH_FILENAME%.*}.patch
	echo $PATCH_FILENAME
	ssh -i $WHISTIUM_INSTANCE_SSH_KEY ubuntu@$WHISTIUM_INSTANCE_IP "cd $WHISTIUM_INSTANCE_CHROMIUM_PATH && git diff $filepath" > $PATCH_FOLDER/$PATCH_FILENAME
done

echo "-------"

# Copy added files
echo "Copying added files..."
ADDED_FOLDER=$MONOREPO_PATH/mandelboxes/browsers/whistium/custom_files
if [ ! -d $ADDED_FOLDER ]; then
	mkdir -p $ADDED_FOLDER
fi
for filepath in $(ssh -i $WHISTIUM_INSTANCE_SSH_KEY ubuntu@$WHISTIUM_INSTANCE_IP "cd $WHISTIUM_INSTANCE_CHROMIUM_PATH && git status --porcelain -uall | awk 'match(\$1, \"\\\\?\\\\?\"){print \$2}'") ; do
	ADDED_FILENAME=$(echo $filepath | tr "/" "-")
	# cp $CHROMIUM_PATH/$filepath $ADDED_FOLDER/$ADDED_FILENAME
	scp -i $WHISTIUM_INSTANCE_SSH_KEY ubuntu@$WHISTIUM_INSTANCE_IP:$WHISTIUM_INSTANCE_CHROMIUM_PATH/$filepath $ADDED_FOLDER/$ADDED_FILENAME
	echo "---copied as $ADDED_FILENAME"
done