#!/bin/bash

# Exit on subcommand errors
set -Eeuo pipefail

# variables below are to be set in the config file and imported
logfile=/home/ubuntu/logfile.log
aws_access_key_id="fill here"
aws_secret_access_key="fill here"

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
  sudo apt-get -y update
  if ! command -v aws &> /dev/null
  then
    echo "AWS-CLI not yet installed. Installing using apt-get..."
    sudo apt-get install -y awscli || echo "Installing AWS-CLI using apt-get failed. This usually happens when the Ubuntu package lists are being updated." && exit
  fi
  aws configure set aws_access_key_id "$aws_access_key_id"
  aws configure set aws_secret_access_key "$aws_secret_access_key"
  echo "AWS configuration is now complete!"
}

clone_whist_repository() {
  git_branch=${1}
  git_token=${2}
  echo "Cloning branch ${git_branch} of the whisthq/whist repository on the AWS instance..."
  rm -rf whist; git clone -b ${git_branch} https://${git_token}@github.com/whisthq/whist.git
}

run_host_setup() {
  cd ~/whist/host-setup && ./setup_host.sh --localdevelopment
}


prune_containers_if_needed() {
  check = $(df -h | grep --color=never /dev/root)
  # process here
  prune_needed=0
  if [[ $post_reboot -eq 0 ]]; then
    echo "Disk is more than 75% full, pruning the docker containers..."
    docker system prune -af
  else
    echo "Disk is less than 75% full, no need to prune containers."
  fi
}

build_client_mandelbox() {
  test_duration=${1}
  cmake_build_type=${2}
  echo "Setting the experiment duration to ${test_duration}s..."
  sed -i "s/timeout 240s/timeout ${test_duration}s/g" ~/whist/mandelboxes/development/client/run-whist-client.sh
  cd ~/whist/mandelboxes && ./build.sh development/client --${cmake_build_type}
}

build_server_mandelbox() {
  cmake_build_type=${2}
  cd ~/whist/mandelboxes && ./build.sh browsers/chrome --${cmake_build_type}
}

main() {
  pre_reboot=${1}
  is_client=${2}
  if [[ $pre_reboot -eq 1 ]]; then
    prune_containers_if_needed
    prepare_for_host_setup
    install_and_configure_aws
    clone_whist_repository
    prepare_for_host_setup
    run_host_setup
    sudo reboot
  else
    if [[ $is_client -eq 1 ]]; then
      build_client_mandelbox
    else
      build_server_mandelbox
    fi
  fi
}
