#!/bin/bash

# Build array of GPU IDs
DRIVER_VERSION=$(modinfo nvidia --field version)
IFS="\n"
IDS=()
for x in `nvidia-smi -L`; do
  IDS+=$(echo "$x" | cut -f6 -d " " | cut -c 1-40)
done

# Convert GPU IDs to JSON Array
ID_JSON=$(printf '%s\n' "${{IDS[@]}}" | jq -R . | jq -s -c .)

# Create JSON GPU Object and populate nvidia-gpu-info.json
echo "{{\"DriverVersion\":\"${{DRIVER_VERSION}}\",\"GPUIDs\":${{ID_JSON}}}}" > /var/lib/ecs/gpu/nvidia-gpu-info.json

#Create ECS config
cat << EOF > /etc/ecs/ecs.config
ECS_CLUSTER={}
ECS_DATADIR=/data
ECS_ENABLE_TASK_IAM_ROLE=true
ECS_ENABLE_TASK_IAM_ROLE_NETWORK_HOST=true
ECS_LOGFILE=/log/ecs-agent.log
ECS_AVAILABLE_LOGGING_DRIVERS=["syslog","json-file","journald","awslogs"]
ECS_LOGLEVEL=info
ECS_ENABLE_GPU_SUPPORT=true
ECS_NVIDIA_RUNTIME=nvidia
ECS_SELINUX_CAPABLE=true
EOF

systemctl stop docker-container@ecs-agent.service
rm -f /var/lib/ecs/data/*
systemctl start docker-container@ecs-agent.service

cd /home/ubuntu
echo export USE_PROD_SENTRY=1 >> /etc/bash.bashrc
chmod +x userdata-bootstrap.sh
./userdata-bootstrap.sh
