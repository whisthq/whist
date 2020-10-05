#!/bin/bash

sudo apt-get remove docker docker-engine docker.io containerd runc
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
sudo usermod -aG docker $USER

sudo apt-get install -y linux-headers-$(uname -r)
distribution=$(. /etc/os-release;echo $ID$VERSION_ID | sed -e 's/\.//g')
wget https://developer.download.nvidia.com/compute/cuda/repos/$distribution/x86_64/cuda-$distribution.pin
sudo mv cuda-$distribution.pin /etc/apt/preferences.d/cuda-repository-pin-600
sudo apt-key adv --fetch-keys https://developer.download.nvidia.com/compute/cuda/repos/$distribution/x86_64/7fa2af80.pub
echo "deb http://developer.download.nvidia.com/compute/cuda/repos/$distribution/x86_64 /" | sudo tee /etc/apt/sources.list.d/cuda.list
sudo apt-get update
sudo apt-mark unhold nvidia-dkms-450
sudo apt-mark unhold nvidia-driver-450
sudo apt-mark unhold nvidia-settings
sudo apt-mark unhold nvidia-modprobe
sudo apt-mark unhold cuda-drivers-450
sudo apt-mark unhold cuda-drivers
sudo apt-get update && sudo apt-get install -y --no-install-recommends --allow-downgrades \
    nvidia-dkms-450=450.80.02-0ubuntu1 \
    nvidia-driver-450=450.80.02-0ubuntu1 \
    nvidia-settings=450.80.02-0ubuntu1 \
    nvidia-modprobe=450.80.02-0ubuntu1 \
    cuda-drivers-450=450.80.02-1 \
    cuda-drivers=450.80.02-1
sudo apt-mark hold nvidia-dkms-450
sudo apt-mark hold nvidia-driver-450
sudo apt-mark hold nvidia-settings
sudo apt-mark hold nvidia-modprobe
sudo apt-mark hold cuda-drivers-450
sudo apt-mark hold cuda-drivers
export PATH=/usr/local/cuda-11.0/bin${PATH:+:${PATH}}
distribution=$(. /etc/os-release;echo $ID$VERSION_ID)
curl -s -L https://nvidia.github.io/nvidia-docker/gpgkey | sudo apt-key add -
curl -s -L https://nvidia.github.io/nvidia-docker/$distribution/nvidia-docker.list | sudo tee /etc/apt/sources.list.d/nvidia-docker.list
sudo apt-get update
sudo apt-get install -y nvidia-docker2
sudo systemctl restart docker

wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ bionic main'
sudo apt-get update
sudo apt-get install -y kitware-archive-keyring
sudo rm /etc/apt/trusted.gpg.d/kitware.gpg
sudo apt-get install -y cmake

echo
echo "Would you like to setup ECS? (y/n)"
read -p "Would you like to setup ECS? " -n 1 -r
echo

if [[ $REPLY =~ ^[Yy]$ ]]; then

  sudo sh -c "echo 'net.ipv4.conf.all.route_localnet = 1' >> /etc/sysctl.conf"
  sudo sysctl -p /etc/sysctl.conf
  echo iptables-persistent iptables-persistent/autosave_v4 boolean true | sudo debconf-set-selections
  echo iptables-persistent iptables-persistent/autosave_v6 boolean true | sudo debconf-set-selections
  sudo apt-get install -y iptables-persistent
  sudo iptables -t nat -A PREROUTING -p tcp -d 169.254.170.2 --dport 80 -j DNAT --to-destination 127.0.0.1:51679
  sudo iptables -t nat -A OUTPUT -d 169.254.170.2 -p tcp -m tcp --dport 80 -j REDIRECT --to-ports 51679
  sudo sh -c 'iptables-save > /etc/iptables/rules.v4'

  sudo mkdir -p /etc/ecs && sudo touch /etc/ecs/ecs.config
  cat << EOF | sudo tee /etc/ecs/ecs.config
ECS_DATADIR=/data
ECS_ENABLE_TASK_IAM_ROLE=true
ECS_ENABLE_TASK_IAM_ROLE_NETWORK_HOST=true
ECS_LOGFILE=/log/ecs-agent.log
ECS_AVAILABLE_LOGGING_DRIVERS=["json-file","awslogs"]
ECS_LOGLEVEL=info
EOF

  sudo docker stop ecs-agent
  sudo docker rm ecs-agent
  sudo docker run --name ecs-agent \
  --detach=true \
  --restart=on-failure:10 \
  --volume=/var/run:/var/run \
  --volume=/var/log/ecs/:/log \
  --volume=/var/lib/ecs/data:/data \
  --volume=/etc/ecs:/etc/ecs \
  --net=host \
  --env-file=/etc/ecs/ecs.config \
  amazon/amazon-ecs-agent:latest
fi

echo
echo 'Install complete. Please "sudo reboot" before continuing'
echo

