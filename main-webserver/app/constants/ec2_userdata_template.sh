#!/bin/bash

# Exit on subcommand errors
set -Eeuo pipefail

# Build array of GPU IDs
DRIVER_VERSION=$(modinfo nvidia --field version)
IFS="\n"
IDS=()
for x in $(nvidia-smi -L); do
    IDS+=$(echo "$x" | cut -f6 -d " " | cut -c 1-40)
done

# Convert GPU IDs to JSON Array
ID_JSON=$(printf '%s\n' "${{IDS[@]}}" | jq -R . | jq -s -c .)

# Create JSON GPU Object and populate nvidia-gpu-info.json
echo "{{\"DriverVersion\":\"${{DRIVER_VERSION}}\",\"GPUIDs\":${{ID_JSON}}}}" > /var/lib/ecs/gpu/nvidia-gpu-info.json

#Create ECS config
sudo mkdir -p /etc/ecs && sudo touch /etc/ecs/ecs.config
cat << EOF > /etc/ecs/ecs.config
ECS_CLUSTER={}
ECS_DATADIR=/var/lib/ecs/data
ECS_ENABLE_TASK_IAM_ROLE=true
ECS_ENABLE_TASK_IAM_ROLE_NETWORK_HOST=true
ECS_LOGFILE=/var/log/ecs/ecs-agent.log
ECS_AVAILABLE_LOGGING_DRIVERS=["syslog","json-file","journald","awslogs"]
ECS_LOGLEVEL=info
ECS_ENABLE_GPU_SUPPORT=true
ECS_NVIDIA_RUNTIME=nvidia
ECS_SELINUX_CAPABLE=true
EOF

rm -f /var/lib/ecs/data/*

cd /home/ubuntu

# The ECS Host Service gets built in the `fractal-build-and-deploy.yml` workflow and
# uploaded from this Git repository to the AMI during Packer via ami_config.json
# Here, we write the systemd unit file for the Fractal ECS Host Service.
# Note that we do not restart the host service. This is because if the host
# service dies for some reason, it is not safe to restart it, since we cannot
# tell the status of the existing cloud storage directories. We just need to
# wait for them to unmount (that is an asynchronous process) and the webserver
# should declare the host dead and prune it.
sudo cat << EOF > /etc/systemd/system/ecs-host-service.service
[Unit]
Description=ECS Host Service
Requires=docker.service
After=docker.service

[Service]
Restart=no
User=root
Type=exec
EnvironmentFile=/usr/share/fractal/app_env.env
ExecStart=/home/ubuntu/ecs-host-service

[Install]
WantedBy=multi-user.target
EOF

# Reload daemon files
sudo /bin/systemctl daemon-reload

# Enable ECS Host Service
sudo systemctl enable --now ecs-host-service.service
