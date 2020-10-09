#!/bin/bash
set -Eeuo pipefail

# If we run this script as root, then the "ubuntu"/default user will not be
# added to the "docker" group, only root will.
if [ "$EUID" -eq 0 ]
    then echo "Do not run this script as root!"
    exit
fi

echo "================================================"
echo "Replacing potentially outdated docker runtime..."
echo "================================================"
# Allow failure with ||:
sudo apt-get remove docker docker-engine docker.io containerd runc ||:
sudo apt-get update
sudo apt-get install -y apt-transport-https ca-certificates curl wget gnupg-agent software-properties-common
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -
sudo apt-key fingerprint 0EBFCD88
sudo add-apt-repository \
   "deb [arch=amd64] https://download.docker.com/linux/ubuntu \
   $(lsb_release -cs) \
   stable"
sudo apt-get update -y
sudo apt-get install -y docker-ce docker-ce-cli containerd.io

# Docker group might already exist, so we allow failure with "||:"
sudo groupadd docker ||:
sudo gpasswd -a $USER docker

echo "================================================"
echo "Configuring docker daemon..."
echo "================================================"
sudo cp docker-daemon-config/daemon.json /etc/docker/daemon.json
sudo cp docker-daemon-config/seccomp-filter.json /etc/docker/seccomp-filter.json

echo "================================================"
echo "Installing nvidia drivers..."
echo "================================================"
sudo apt-get install -y linux-headers-$(uname -r)
distribution=$(. /etc/os-release;echo $ID$VERSION_ID | sed -e 's/\.//g')
wget https://developer.download.nvidia.com/compute/cuda/repos/$distribution/x86_64/cuda-$distribution.pin
sudo mv cuda-$distribution.pin /etc/apt/preferences.d/cuda-repository-pin-600
sudo apt-key adv --fetch-keys https://developer.download.nvidia.com/compute/cuda/repos/$distribution/x86_64/7fa2af80.pub
echo "deb http://developer.download.nvidia.com/compute/cuda/repos/$distribution/x86_64 /" | sudo tee /etc/apt/sources.list.d/cuda.list
sudo apt-get update
sudo apt-mark unhold \
    nvidia-dkms-450 \
    nvidia-driver-450 \
    nvidia-settings \
    nvidia-modprobe \
    cuda-drivers-450 \
    cuda-drivers
sudo apt-get update && sudo apt-get install -y --no-install-recommends --allow-downgrades \
    nvidia-dkms-450=450.80.02-0ubuntu1 \
    nvidia-driver-450=450.80.02-0ubuntu1 \
    nvidia-settings=450.80.02-0ubuntu1 \
    nvidia-modprobe=450.80.02-0ubuntu1 \
    cuda-drivers-450=450.80.02-1 \
    cuda-drivers=450.80.02-1
sudo apt-mark hold \
    nvidia-dkms-450 \
    nvidia-driver-450 \
    nvidia-settings \
    nvidia-modprobe \
    cuda-drivers-450 \
    cuda-drivers
export PATH=/usr/local/cuda-11.0/bin${PATH:+:${PATH}}

echo "================================================"
echo "Installing nvidia-docker..."
echo "Note that (as of 10/5/20) the URLs may still say 18.04. This is because"
echo "NVIDIA has redirected the corresponding 20.04 URLs to the 18.04 versions."
echo "================================================"
distribution=$(. /etc/os-release;echo $ID$VERSION_ID)
curl -s -L https://nvidia.github.io/nvidia-docker/gpgkey | sudo apt-key add -
curl -s -L https://nvidia.github.io/nvidia-docker/$distribution/nvidia-docker.list | sudo tee /etc/apt/sources.list.d/nvidia-docker.list
sudo apt-get update
sudo apt-get install -y nvidia-docker2
sudo systemctl enable docker
sudo systemctl restart docker

echo "================================================"
echo "Installing AWS CLI..."
echo "================================================"
sudo apt install awscli

echo "================================================"
echo "Cleaning up the image a bit..."
echo "================================================"
sudo apt autoremove

echo
echo 'Install complete. Please "sudo reboot" before continuing'
echo
