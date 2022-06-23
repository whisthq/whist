#!/bin/bash

# Exit on subcommand errors
set -Eeuo pipefail

# variables below are to be set in the config file and imported
logfile=/home/ubuntu/logfile.log
aws_access_key_id="fill here"
aws_secret_access_key="fill here"
test_duration=126
cmake_build_type="metrics"
skip_git_clone=0
role="client"
git_branch="dev"
git_token="ajdbwjhefbwjkfbwjw"

run_cmd() {
  command=${1}
  cmd_stdout=$(eval "$command" | tee --append "$logfile")
  # Add newline
  echo >> "$logfile"
}

wait_for_apt_locks() {
  run_cmd "while sudo fuser /var/{lib/{dpkg,apt/lists},cache/apt/archives}/lock >/dev/null 2>&1; do echo 'Waiting for apt locks...'; sleep 1; done"
}

prepare_for_host_setup() {
  # Set dkpg frontend as non-interactive to avoid irrelevant warnings
  run_cmd "echo 'debconf debconf/frontend select Noninteractive' | sudo debconf-set-selections"
  wait_for_apt_locks
  # Clean, upgrade and update all the apt lists
  run_cmd "sudo apt-get clean -y && sudo apt-get upgrade -y && sudo apt-get update -y"
}

install_and_configure_aws() {
  wait_for_apt_locks
  run_cmd "sudo apt-get -y update"
  if ! command -v aws &> /dev/null
  then
    echo "AWS-CLI not yet installed. Installing using apt-get..."
    sudo apt-get install -y awscli || echo "Installing AWS-CLI using apt-get failed. This usually happens when the Ubuntu package lists are being updated." && exit
  fi
  run_cmd "aws configure set aws_access_key_id $aws_access_key_id"
  run_cmd "aws configure set aws_secret_access_key $aws_secret_access_key"
  echo "AWS configuration is now complete!"
}

clone_whist_repository() {
  echo "Cloning branch ${git_branch} of the whisthq/whist repository on the AWS instance..."
  run_cmd "rm -rf whist; git clone -b ${git_branch} https://${git_token}@github.com/whisthq/whist.git"
}

run_host_setup() {
  wait_for_apt_locks
  run_cmd "cd ~/whist/host-setup && ./setup_host.sh --localdevelopment"
}


prune_containers_if_needed() {
  check = $(df -h | grep --color=never /dev/root)
  # process here
  prune_needed=0
  if [[ $post_reboot -eq 0 ]]; then
    echo "Disk is more than 75% full, pruning the docker containers..."
    run_cmd "docker system prune -af"
  else
    echo "Disk is less than 75% full, no need to prune containers."
  fi
}

build_client_mandelbox() {
  echo "Setting the experiment duration to ${test_duration}s..."
  run_cmd "sed -i \'s/timeout 240s/timeout ${test_duration}s/g\' ~/whist/mandelboxes/development/client/run-whist-client.sh"
  run_cmd "cd ~/whist/mandelboxes && ./build.sh development/client --${cmake_build_type}"
}

build_server_mandelbox() {
  run_cmd "cd ~/whist/mandelboxes && ./build.sh browsers/chrome --${cmake_build_type}"
}

restore_network_conditions() {

}

main() {
  pre_reboot=${1}
  if [[ $pre_reboot -eq 1 ]]; then
    if [ "$role" == "client" ]; then restore_network_conditions; fi
    prepare_for_host_setup
    prune_containers_if_needed
    install_and_configure_aws
    if [ $skip_git_clone -eq 0 ]; then clone_whist_repository; fi
    if [ $skip_host_setup -eq 0 ]; then run_host_setup; fi
    sudo reboot
  else
    if [[ "$role" == "client" ]]; then
      build_client_mandelbox
    else
      build_server_mandelbox
    fi
  fi
}
