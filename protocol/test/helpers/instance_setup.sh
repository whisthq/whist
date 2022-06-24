#!/bin/bash

# Exit on subcommand errors
set -Eeuo pipefail

# variables below are to be set in the config file and imported
. instance_setup_configs.sh
#logfile=/home/ubuntu/logfile.log
#aws_access_key_id="fill here"
#aws_secret_access_key="fill here"
#testing_time=126
#cmake_build_type="metrics"
#skip_git_clone=0
#role="client"
#branch_name="dev"
#github_token="ajdbwjhefbwjkfbwjw"
#disk_full_threshold=75

run_cmd() {
  command=${1}
  cmd_stdout=$(eval "$command" | tee --append "$logfile")
  ret=$? # TODO: fix this to make the ret variable point to the "$command" exit code 
  # Add newline to logfile
  echo >> "$logfile"
}

handle_errors() {
  errors_to_search=${1}
  error_messages=${2}
  corrective_actions=${3}

  local index=0
  for err in ${errors_to_search[*]}; do
    if [[ $cmd_stdout == *"$err"* ]]; then
      echo "${error_messages[$index]}"
      if [[ ! -z "${corrective_actions[$index]}" ]]; then
        echo "Attempting to fix with command ${corrective_actions[$index]}"
        run_cmd "${corrective_actions[$index]}"
      fi
      break
    fi
    (( index++ ))
  done
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
    run_cmd "sudo apt-get install -y awscli"
    # || echo "Installing AWS-CLI using apt-get failed. This usually happens when the Ubuntu package lists are being updated." && exit
  fi
  run_cmd "aws configure set aws_access_key_id $aws_access_key_id"
  run_cmd "aws configure set aws_secret_access_key $aws_secret_access_key"
  echo "AWS configuration is now complete!"
}

clone_whist_repository() {
  echo "Cloning branch ${branch_name} of the whisthq/whist repository on the AWS instance..."
  run_cmd "rm -rf whist; git clone -b ${branch_name} https://${github_token}@github.com/whisthq/whist.git"
}

run_host_setup() {
  echo "Running the host setup..."
  wait_for_apt_locks
  run_cmd "cd ~/whist/host-setup && ./setup_host.sh --localdevelopment"
}


prune_containers_if_needed() {
  run_cmd "df --output=pcent /dev/root | grep -v Use | sed 's/%//'"
  disk_usage="$cmd_stdout"
  if [[ "$disk_usage" -gt "$disk_full_threshold" ]]; then
    echo "Disk is more than 75% full, pruning the docker containers..."
    run_cmd "docker system prune -af"
  else
    echo "Disk is less than 75% full, no need to prune containers."
  fi
}

build_mandelboxes() {
  if [[ "$role" == "server" || "$role" == "both" ]]; then
    echo "Building the server mandelbox in ${cmake_build_type} mode"
    run_cmd "cd ~/whist/mandelboxes && ./build.sh browsers/chrome --${cmake_build_type}"
    echo "Finished building the server mandelbox!"
  fi
  
  if [[ "$role" == "client" || "$role" == "both" ]]; then
    echo "Setting the experiment duration to ${testing_time}s..."
    run_cmd "sed -i \'s/timeout 240s/timeout ${testing_time}s/g\' ~/whist/mandelboxes/development/client/run-whist-client.sh"
    echo "Building the dev client mandelbox in ${cmake_build_type} mode..."
    run_cmd "cd ~/whist/mandelboxes && ./build.sh development/client --${cmake_build_type}"
    echo "Finished building the dev client mandelbox!"
  fi
}

restore_network_conditions_if_needed() {
  if [[ "$role" == "server" ]]; then 
    return
  fi
  # Check that ifconfig is installed
  if ! command -v aws &> /dev/null
  then
    echo "ifconfig is not installed on the client instance, so we don't need to restore normal network conditions."
    return
  fi
  # Kill the script to apply net conditions if it is still running
  run_cmd "killall -9 -v apply_network_conditions.sh"
  # Get the list of network devices
  devices=($(basename -a /sys/class/net/* ))
  for device in ${devices[*]}; do
    if [[ $device == *"docker"* || $device == *"veth"* || $device == *"lo"* || $device == *"ifb"* ]]; then
      continue
    else
      echo "Restoring normal network conditions on device ${device}"
      # Inbound degradations
      run_cmd "sudo tc qdisc del dev ${device} handle ffff: ingress"
      # Outbound degradations
      run_cmd "sudo tc qdisc del dev ${device} root netem"
    fi
  done
  run_cmd "sudo modprobe -r ifb"
}

reboot_machine() {
  echo "Rebooting the machine..."
  sudo reboot
}

main() {
  pre_reboot=${1}
  if [[ $pre_reboot -eq 1 ]]; then
    restore_network_conditions_if_needed
    prepare_for_host_setup
    prune_containers_if_needed
    install_and_configure_aws
    if [[ $skip_git_clone == "true" ]]; then 
      clone_whist_repository
    fi
    if [[ $skip_host_setup == "true" ]]; then 
      run_host_setup
    fi
    reboot_machine
  else
    build_mandelboxes
    echo "Done!"
  fi
}

main ${1}

