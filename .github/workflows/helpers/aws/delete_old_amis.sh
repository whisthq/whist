#!/bin/bash

dry_run=1
echo_progress=1

d=$(date +'%Y-%m-%d' -d '1 month ago')
if [ $echo_progress -eq 1 ]
then
  echo "Date of snapshots to delete (if older than): $d"
fi

snapshots_to_delete=$(aws ec2 describe-snapshots \
    --owner-ids xxxxxxxxxxxxx \
    --output text \
    --query "Snapshots[?StartTime<'$d'].SnapshotId" \
)
if [ $echo_progress -eq 1 ]
then
  echo "List of snapshots to delete: $snapshots_to_delete"
fi


for oldsnap in $snapshots_to_delete; do

  # some $oldsnaps will be in use, so you can't delete them
  # for "snap-a1234xyz" currently in use by "ami-zyx4321ab"
  # (and others it can't delete) add conditionals like this

  if [ "$oldsnap" = "snap-a1234xyz" ] ||
     [ "$oldsnap" = "snap-c1234abc" ]
  then
    if [ $echo_progress -eq 1 ]
    then
       echo "skipping $oldsnap known to be in use by an ami"
    fi
    continue
  fi

  if [ $echo_progress -eq 1 ]
  then
     echo "deleting $oldsnap"
  fi

  if [ $dry_run -eq 1 ]
  then
    # dryrun will not actually delete the snapshots
    aws ec2 delete-snapshot --snapshot-id $oldsnap --dry-run
  else
    aws ec2 delete-snapshot --snapshot-id $oldsnap
  fi
done