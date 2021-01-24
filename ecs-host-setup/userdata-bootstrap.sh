#!/bin/bash

# This script installs and enables the Fractal ECS Host Service on an AMI
# It gets run from the webserver at EC2 instance creation

echo "Running userdata-bootstrap.sh"

# Retrieve Fractal ECS Host Service binary from AWS S3
aws s3 cp s3://fractal-ecs-host-service/ecs-host-service ecs-host-service
chmod +x ecs-host-service

# Write systemd unit file for Fractal ECS Host Service
sudo cat << EOF > /etc/systemd/system/ecs-host-service.service
[Unit]
Description=ECS Host Service
Requires=docker.service
After=docker.service

[Service]
Restart=always
User=root
Type=exec
Environment="APP_ENV=PROD"
ExecStart=/home/ubuntu/ecs-host-service

[Install]
WantedBy=multi-user.target
EOF

# Reload daemon files
sudo /bin/systemctl daemon-reload

# Enable ECS Host Service
sudo systemctl enable --now ecs-host-service.service
