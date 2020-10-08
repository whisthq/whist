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

# Create directories for ECS agent
sudo mkdir -p /var/log/ecs /var/lib/ecs/{data,gpu} /etc/ecs

# Install jq to build JSON
sudo apt install -y jq

# Create list of GPU devices as 
DEVICES=""
for DEVICE_INDEX in {0..64}
do
  DEVICE_PATH="/dev/nvidia${DEVICE_INDEX}"
  if [ -e "$DEVICE_PATH" ]; then
    DEVICES="${DEVICES} --device ${DEVICE_PATH}:${DEVICE_PATH} "
  fi
done
DEVICE_MOUNTS=`printf "$DEVICES"`

echo "================================================"
echo "Cleaning up the image a bit..."
echo "================================================"
sudo apt autoremove

echo "================================================"
echo "Installing AWS CLI..."
echo "================================================"
sudo apt-get install awscli

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
ECS_CLUSTER=default
ECS_DATADIR=/data
ECS_ENABLE_TASK_IAM_ROLE=true
ECS_ENABLE_TASK_IAM_ROLE_NETWORK_HOST=true
ECS_LOGFILE=/log/ecs-agent.log
ECS_AVAILABLE_LOGGING_DRIVERS=["syslog", "json-file", "journald", "awslogs"]
ECS_LOGLEVEL=info
ECS_ENABLE_GPU_SUPPORT=true
ECS_NVIDIA_RUNTIME=nvidia
EOF

  # Write systemd unit file for ECS Agent
  cat << EOF > /etc/systemd/system/docker-container@ecs-agent.service
[Unit]
Description=Docker Container %I
Requires=docker.service
After=docker.service

[Service]
Restart=always
ExecStartPre=-/usr/bin/docker rm -f %i
ExecStart=/usr/bin/docker run --name %i \
--init \
--restart=on-failure:10 \
--volume=/var/run:/var/run \
--volume=/var/log/ecs/:/log \
--volume=/var/lib/ecs/data:/data \
--volume=/etc/ecs:/etc/ecs \
--volume=/sbin:/host/sbin \
--volume=/lib:/lib \
--volume=/lib64:/lib64 \
--volume=/usr/lib:/usr/lib \
--volume=/usr/lib64:/usr/lib64 \
--volume=/proc:/host/proc \
--volume=/sys/fs/cgroup:/sys/fs/cgroup \
--net=host \
--env-file=/etc/ecs/ecs.config \
--cap-add=sys_admin \
--cap-add=net_admin \
--volume=/var/lib/nvidia-docker/volumes/nvidia_driver/latest:/usr/local/nvidia \
--device /dev/nvidiactl:/dev/nvidiactl \
${DEVICE_MOUNTS} \
--device /dev/nvidia-uvm:/dev/nvidia-uvm \
--volume=/var/lib/ecs/gpu:/var/lib/ecs/gpu \
amazon/amazon-ecs-agent:latest
ExecStop=/usr/bin/docker stop %i

[Install]
WantedBy=default.target
EOF

  # Reload daemon files
  sudo /bin/systemctl daemon-reload

  # Enabling ECS Agent
  sudo systemctl enable docker-container@ecs-agent.service

  sudo rm -rf /var/lib/cloud/instances/
  sudo rm -f /var/lib/ecs/data/*
fi

echo
echo 'Install complete. Make sure you do not reboot when creating AMI (check NO REBOOT)'
echo
