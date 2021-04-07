#!/bin/bash

set -Eeuo pipefail

# Parse input variables
GH_PAT=${1}
GH_USERNAME=${2} 
GIT_BRANCH=${3}
GIT_HASH=${4}
# Set dkpg frontend as non-interactive to avoid irrelevant warnings
echo 'debconf debconf/frontend select Noninteractive' | sudo debconf-set-selections
sudo apt-get install -y -q

echo "================================================"
echo "Replacing potentially outdated Docker runtime..."
echo "================================================"

# Attempt to remove potentially oudated Docker runtime
# Allow failure with ||:, in case they're not installed yet
sudo apt-get remove docker docker-engine docker.io containerd runc ||:
sudo apt-get clean
sudo apt-get upgrade
sudo apt-get update

# Install latest Docker runtime and dependencies
sudo apt-get install -y apt-transport-https ca-certificates curl wget gnupg-agent software-properties-common
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -
sudo apt-key fingerprint 0EBFCD88
sudo add-apt-repository \
    "deb [arch=amd64] https://download.docker.com/linux/ubuntu \
   $(lsb_release -cs) \
    stable"
sudo apt-get update -y
sudo apt-get install -y docker-ce docker-ce-cli containerd.io

# Attempt to Add Docker group, but allow failure with "||:" in case it already exists
sudo groupadd docker ||:
sudo gpasswd -a "$USER" docker

ghcr_uri=ghcr.io
echo $GH_PAT | docker login --username $GH_USERNAME --password-stdin $ghcr_uri